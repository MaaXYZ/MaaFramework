#include "HdrDisplayUtils.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "MaaUtils/Logger.h"

namespace
{
constexpr LONG kDisplayConfigErrorInsufficientBuffer = 122;
constexpr int kMaxDisplayConfigRetries = 3;
constexpr float kDefaultSdrWhiteNits = 80.0f;
constexpr float kSrgbToLinearThreshold = 0.04045f;
constexpr float kLinearToSrgbThreshold = 0.0031308f;
constexpr float kInvSrgbGamma = 1.0f / 2.4f;
constexpr DISPLAYCONFIG_DEVICE_INFO_TYPE kDisplayConfigDeviceInfoGetAdvancedColorInfo2 =
    static_cast<DISPLAYCONFIG_DEVICE_INFO_TYPE>(15);
constexpr UINT32 kAdvancedColorActiveMask = 0x2u;
constexpr UINT32 kHdrUserEnabledMask = 0x20u;
constexpr int kAdvancedColorModeHdr = 2;

float srgb_to_linear(float srgb)
{
    srgb = std::clamp(srgb, 0.0f, 1.0f);
    if (srgb <= kSrgbToLinearThreshold) {
        return srgb / 12.92f;
    }
    return std::pow((srgb + 0.055f) / 1.055f, 2.4f);
}

uint8_t linear_to_srgb_u8(float linear)
{
    linear = std::clamp(linear, 0.0f, 1.0f);
    const float srgb = linear <= kLinearToSrgbThreshold ? linear * 12.92f : 1.055f * std::pow(linear, kInvSrgbGamma) - 0.055f;
    return static_cast<uint8_t>(std::lround(std::clamp(srgb, 0.0f, 1.0f) * 255.0f));
}

using DeviceNameBuffer = std::array<wchar_t, CCHDEVICENAME>;

std::optional<DeviceNameBuffer> get_monitor_device_name(HMONITOR target_monitor)
{
    if (!target_monitor) {
        return std::nullopt;
    }

    MONITORINFOEXW monitor_info {};
    monitor_info.cbSize = sizeof(monitor_info);
    if (!GetMonitorInfoW(target_monitor, &monitor_info)) {
        LogWarn << "GetMonitorInfoW failed" << GetLastError();
        return std::nullopt;
    }

    DeviceNameBuffer device_name {};
    std::copy_n(monitor_info.szDevice, CCHDEVICENAME, device_name.begin());
    return device_name;
}

bool device_name_equals(const wchar_t (&lhs)[CCHDEVICENAME], const DeviceNameBuffer& rhs)
{
    for (size_t i = 0; i < CCHDEVICENAME; ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
        if (lhs[i] == L'\0') {
            return true;
        }
    }
    return true;
}

bool device_name_equals(const DeviceNameBuffer& lhs, const DeviceNameBuffer& rhs)
{
    for (size_t i = 0; i < CCHDEVICENAME; ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
        if (lhs[i] == L'\0') {
            return true;
        }
    }
    return true;
}

struct DisplayConfigGetAdvancedColorInfo2
{
    DISPLAYCONFIG_DEVICE_INFO_HEADER header {};
    UINT32 value = 0;
    DISPLAYCONFIG_COLOR_ENCODING colorEncoding = DISPLAYCONFIG_COLOR_ENCODING_RGB;
    UINT32 bitsPerColorChannel = 0;
    INT activeColorMode = 0;
};

struct PathHdrState
{
    bool valid = false;
    bool hdr_user_enabled = false;
    bool hdr_enabled = false;
    int active_color_mode = 0;
};

struct ActiveDisplayPathInfo
{
    DISPLAYCONFIG_PATH_INFO path {};
    std::optional<DeviceNameBuffer> source_device_name;
    PathHdrState hdr_state;
};

std::optional<DeviceNameBuffer> query_source_device_name(const DISPLAYCONFIG_PATH_INFO& path)
{
    DISPLAYCONFIG_SOURCE_DEVICE_NAME source_name {};
    source_name.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    source_name.header.size = sizeof(DISPLAYCONFIG_SOURCE_DEVICE_NAME);
    source_name.header.adapterId = path.sourceInfo.adapterId;
    source_name.header.id = path.sourceInfo.id;

    const LONG ret = DisplayConfigGetDeviceInfo(&source_name.header);
    if (ret != ERROR_SUCCESS) {
        return std::nullopt;
    }

    DeviceNameBuffer device_name {};
    std::copy_n(source_name.viewGdiDeviceName, CCHDEVICENAME, device_name.begin());
    return device_name;
}

PathHdrState query_path_hdr_state(const DISPLAYCONFIG_PATH_INFO& path)
{
    DisplayConfigGetAdvancedColorInfo2 color_info2 {};
    color_info2.header.type = kDisplayConfigDeviceInfoGetAdvancedColorInfo2;
    color_info2.header.size = sizeof(DisplayConfigGetAdvancedColorInfo2);
    color_info2.header.adapterId = path.targetInfo.adapterId;
    color_info2.header.id = path.targetInfo.id;

    LONG ret = DisplayConfigGetDeviceInfo(&color_info2.header);
    if (ret == ERROR_SUCCESS) {
        const bool hdr_user_enabled = (color_info2.value & kHdrUserEnabledMask) != 0;
        const bool hdr_active = (color_info2.value & kAdvancedColorActiveMask) != 0
                                && color_info2.activeColorMode == kAdvancedColorModeHdr;
        return {
            .valid = true,
            .hdr_user_enabled = hdr_user_enabled,
            .hdr_enabled = hdr_active,
            .active_color_mode = color_info2.activeColorMode,
        };
    }

    DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO color_info {};
    color_info.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO;
    color_info.header.size = sizeof(DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO);
    color_info.header.adapterId = path.targetInfo.adapterId;
    color_info.header.id = path.targetInfo.id;

    ret = DisplayConfigGetDeviceInfo(&color_info.header);
    if (ret != ERROR_SUCCESS) {
        return {};
    }

    const bool hdr_enabled = (color_info.value & kAdvancedColorActiveMask) != 0;
    return {
        .valid = true,
        .hdr_user_enabled = hdr_enabled,
        .hdr_enabled = hdr_enabled,
        .active_color_mode = hdr_enabled ? kAdvancedColorModeHdr : 0,
    };
}

std::optional<std::vector<ActiveDisplayPathInfo>> query_active_display_paths()
{
    for (int attempt = 0; attempt < kMaxDisplayConfigRetries; ++attempt) {
        UINT32 path_count = 0;
        UINT32 mode_count = 0;

        LONG ret = GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &path_count, &mode_count);
        if (ret != ERROR_SUCCESS) {
            LogWarn << "GetDisplayConfigBufferSizes failed" << VAR(ret);
            return std::nullopt;
        }

        if (path_count == 0) {
            return std::vector<ActiveDisplayPathInfo> {};
        }

        std::vector<DISPLAYCONFIG_PATH_INFO> path_array(path_count);
        std::vector<DISPLAYCONFIG_MODE_INFO> mode_array(mode_count);

        ret = QueryDisplayConfig(
            QDC_ONLY_ACTIVE_PATHS,
            &path_count,
            path_array.data(),
            &mode_count,
            mode_array.empty() ? nullptr : mode_array.data(),
            nullptr);
        if (ret == kDisplayConfigErrorInsufficientBuffer) {
            continue;
        }
        if (ret != ERROR_SUCCESS) {
            LogWarn << "QueryDisplayConfig failed" << VAR(ret);
            return std::nullopt;
        }

        std::vector<ActiveDisplayPathInfo> result;
        result.reserve(path_count);
        int success_count = 0;

        for (UINT32 i = 0; i < path_count; ++i) {
            ActiveDisplayPathInfo info {};
            info.path = path_array[i];
            info.source_device_name = query_source_device_name(path_array[i]);
            info.hdr_state = query_path_hdr_state(path_array[i]);
            if (info.hdr_state.valid) {
                ++success_count;
            }
            result.emplace_back(std::move(info));
        }

        if (success_count == 0) {
            LogWarn << "DisplayConfigGetDeviceInfo failed for all active display paths";
            return std::nullopt;
        }

        return result;
    }

    LogWarn << "QueryDisplayConfig kept returning ERROR_INSUFFICIENT_BUFFER for HDR query";
    return std::nullopt;
}

void try_query_sdr_white_level(MAA_CTRL_UNIT_NS::HdrDisplayState& state, const DISPLAYCONFIG_PATH_INFO& path)
{
    DISPLAYCONFIG_SDR_WHITE_LEVEL white_level {};
    white_level.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
    white_level.header.size = sizeof(DISPLAYCONFIG_SDR_WHITE_LEVEL);
    white_level.header.adapterId = path.targetInfo.adapterId;
    white_level.header.id = path.targetInfo.id;

    const LONG ret = DisplayConfigGetDeviceInfo(&white_level.header);
    if (ret == ERROR_SUCCESS) {
        state.sdr_white_nits = std::max(
            kDefaultSdrWhiteNits,
            static_cast<float>((static_cast<double>(white_level.SDRWhiteLevel) / 1000.0) * kDefaultSdrWhiteNits));
    }
    else {
        LogWarn << "DisplayConfigGetDeviceInfo(GET_SDR_WHITE_LEVEL) failed" << VAR(ret);
    }
}

} // namespace

MAA_CTRL_UNIT_NS_BEGIN

std::optional<bool> query_windows_hdr_enabled_go_compatible()
{
    const auto active_paths = query_active_display_paths();
    if (!active_paths.has_value()) {
        return std::nullopt;
    }

    for (const auto& info : *active_paths) {
        if (info.hdr_state.valid && info.hdr_state.hdr_enabled) {
            return true;
        }
    }

    return false;
}

std::optional<HdrDisplayState> query_hdr_display_state(HMONITOR target_monitor)
{
    const auto active_paths = query_active_display_paths();
    if (!active_paths.has_value()) {
        return std::nullopt;
    }

    HdrDisplayState state {
        .hdr_enabled = false,
        .hdr_user_enabled = false,
        .active_color_mode = 0,
        .sdr_white_nits = kDefaultSdrWhiteNits,
        .valid = false,
    };

    if (!target_monitor) {
        bool has_valid_path = false;
        for (const auto& info : *active_paths) {
            if (!info.hdr_state.valid) {
                continue;
            }
            has_valid_path = true;
            state.hdr_enabled = state.hdr_enabled || info.hdr_state.hdr_enabled;
            state.hdr_user_enabled = state.hdr_user_enabled || info.hdr_state.hdr_user_enabled;
            if (info.hdr_state.hdr_enabled) {
                state.active_color_mode = info.hdr_state.active_color_mode;
            }
        }
        state.valid = has_valid_path;
        return state;
    }

    const auto target_device_name = get_monitor_device_name(target_monitor);
    if (!target_device_name.has_value()) {
        LogWarn << "Failed to query target monitor device name for HDR state" << VAR_VOIDP(target_monitor);
        return state;
    }

    bool matched_path = false;
    bool matched_valid_path = false;
    bool white_level_queried = false;
    for (const auto& info : *active_paths) {
        if (!info.source_device_name.has_value() || !device_name_equals(*info.source_device_name, *target_device_name)) {
            continue;
        }
        matched_path = true;

        if (!info.hdr_state.valid) {
            continue;
        }
        matched_valid_path = true;

        state.valid = true;
        state.hdr_enabled = state.hdr_enabled || info.hdr_state.hdr_enabled;
        state.hdr_user_enabled = state.hdr_user_enabled || info.hdr_state.hdr_user_enabled;
        if (info.hdr_state.hdr_enabled || state.active_color_mode == 0) {
            state.active_color_mode = info.hdr_state.active_color_mode;
        }
        if (state.hdr_enabled && !white_level_queried) {
            try_query_sdr_white_level(state, info.path);
            white_level_queried = true;
        }
    }

    if (!matched_path) {
        LogInfo << "No active display path matched target monitor for HDR state query" << VAR_VOIDP(target_monitor);
    }
    else if (!matched_valid_path) {
        LogWarn << "Matched display path lacked valid HDR state; caller may fall back to global query" << VAR_VOIDP(target_monitor);
    }

    return state;
}

cv::Mat compensate_hdr_sdr_capture(const cv::Mat& bgra, float sdr_white_nits)
{
    if (bgra.empty() || bgra.type() != CV_8UC4) {
        return {};
    }

    const float hdr_white_scale = std::max(1.0f, sdr_white_nits / kDefaultSdrWhiteNits);
    cv::Mat bgr8(bgra.rows, bgra.cols, CV_8UC3);

    cv::parallel_for_(cv::Range(0, bgra.rows), [&](const cv::Range& range) {
        for (int y = range.start; y < range.end; ++y) {
            const auto* src_row = bgra.ptr<cv::Vec4b>(y);
            auto* dst_row = bgr8.ptr<cv::Vec3b>(y);

            for (int x = 0; x < bgra.cols; ++x) {
                const float b = srgb_to_linear(src_row[x][0] / 255.0f) / hdr_white_scale;
                const float g = srgb_to_linear(src_row[x][1] / 255.0f) / hdr_white_scale;
                const float r = srgb_to_linear(src_row[x][2] / 255.0f) / hdr_white_scale;

                dst_row[x] = cv::Vec3b(linear_to_srgb_u8(b), linear_to_srgb_u8(g), linear_to_srgb_u8(r));
            }
        }
    });

    return bgr8;
}

MAA_CTRL_UNIT_NS_END
