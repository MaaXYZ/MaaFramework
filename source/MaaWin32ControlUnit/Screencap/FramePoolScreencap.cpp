#include "FramePoolScreencap.h"

#if MAA_FRAMEPOOL_SCREENCAP_AVAILABLE

#include <d3dcompiler.h>
#include <dwmapi.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Security.Authorization.AppCapabilityAccess.h>

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "HdrDisplayUtils.hpp"
#include "HwndUtils.hpp"
#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "MaaUtils/Time.hpp"

namespace
{
constexpr LONG kDisplayConfigErrorInsufficientBuffer = 122;
constexpr int kMaxDisplayConfigRetries = 3;

using CaptureFrame = winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame;

constexpr auto kFramePollInterval = std::chrono::milliseconds(2);
constexpr auto kColdFrameTimeout = std::chrono::milliseconds(500);
constexpr auto kSteadyFrameTimeout = std::chrono::milliseconds(50);
constexpr float kLinearToSrgbThreshold = 0.0031308f;
constexpr float kInvSrgbGamma = 1.0f / 2.4f;
constexpr float kHdrDetectEpsilon = 1e-3f;
constexpr float kMinLuminance = 1e-6f;
constexpr float kHdrMiddleGray = 0.18f;
constexpr float kHdrTargetHighlight = 0.85f;
constexpr float kHdrMinExposure = 1.0f / 32.0f;
constexpr float kHdrMaxExposure = 4.0f;
constexpr int kHdrStatsLongSide = 160;

struct HdrSceneStats
{
    float min_linear = 0.0f;
    float max_linear = 0.0f;
    float avg_log_luminance = 0.0f;
    float highlight_luminance = 0.0f;
    bool valid = false;
};

struct alignas(16) HdrGpuShaderConstants
{
    std::array<float, 2> source_offset {};
    std::array<float, 2> source_scale {};
    float exposure = 1.0f;
    float apply_tone_mapping = 0.0f;
    std::array<float, 2> padding {};
};

cv::Size calc_hdr_stats_size(int width, int height)
{
    if (width <= 0 || height <= 0) {
        return {};
    }

    if (width >= height) {
        const int stats_width = kHdrStatsLongSide;
        const int stats_height = std::max(1, static_cast<int>(std::lround(static_cast<double>(kHdrStatsLongSide) * height / width)));
        return { stats_width, stats_height };
    }

    const int stats_height = kHdrStatsLongSide;
    const int stats_width = std::max(1, static_cast<int>(std::lround(static_cast<double>(kHdrStatsLongSide) * width / height)));
    return { stats_width, stats_height };
}

HdrSceneStats analyze_hdr_scene(const cv::Mat& hdr_rgba)
{
    HdrSceneStats stats;
    if (hdr_rgba.empty()) {
        return stats;
    }

    cv::Mat rgba32f;
    if (hdr_rgba.type() == CV_32FC4) {
        rgba32f = hdr_rgba;
    }
    else {
        hdr_rgba.convertTo(rgba32f, CV_32FC4);
    }

    stats.min_linear = std::numeric_limits<float>::max();
    stats.max_linear = std::numeric_limits<float>::lowest();

    std::vector<float> luminances;
    luminances.reserve(static_cast<size_t>(rgba32f.rows * rgba32f.cols));

    double log_sum = 0.0;
    size_t sample_count = 0;

    for (int y = 0; y < rgba32f.rows; ++y) {
        const auto* row = rgba32f.ptr<cv::Vec4f>(y);
        for (int x = 0; x < rgba32f.cols; ++x) {
            const cv::Vec4f& pixel = row[x];
            for (int c = 0; c < 3; ++c) {
                stats.min_linear = std::min(stats.min_linear, pixel[c]);
                stats.max_linear = std::max(stats.max_linear, pixel[c]);
            }

            const float r = std::max(pixel[0], 0.0f);
            const float g = std::max(pixel[1], 0.0f);
            const float b = std::max(pixel[2], 0.0f);
            const float luminance = std::max(kMinLuminance, 0.2126f * r + 0.7152f * g + 0.0722f * b);
            luminances.emplace_back(luminance);
            log_sum += std::log(luminance);
            ++sample_count;
        }
    }

    if (sample_count == 0 || luminances.empty()) {
        return stats;
    }

    const size_t highlight_index = static_cast<size_t>(std::floor((luminances.size() - 1) * 0.95));
    std::nth_element(luminances.begin(), luminances.begin() + highlight_index, luminances.end());

    stats.avg_log_luminance = static_cast<float>(std::exp(log_sum / static_cast<double>(sample_count)));
    stats.highlight_luminance = luminances[highlight_index];
    stats.valid = true;
    return stats;
}

float calc_hdr_exposure(const HdrSceneStats& stats, bool& apply_tone_mapping)
{
    apply_tone_mapping = stats.valid && (stats.max_linear > 1.0f + kHdrDetectEpsilon || stats.min_linear < -kHdrDetectEpsilon);
    if (!apply_tone_mapping) {
        return 1.0f;
    }

    float exposure = 1.0f;
    if (stats.avg_log_luminance > kMinLuminance) {
        exposure = kHdrMiddleGray / stats.avg_log_luminance;
    }
    if (stats.highlight_luminance > kMinLuminance) {
        exposure = std::min(exposure, kHdrTargetHighlight / stats.highlight_luminance);
    }
    return std::clamp(exposure, kHdrMinExposure, kHdrMaxExposure);
}

constexpr std::string_view kHdrFullscreenVertexShader = R"(
struct VSOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOut main(uint vertex_id : SV_VertexID)
{
    VSOut output;

    float2 positions[3] = {
        float2(-1.0, -1.0),
        float2(-1.0,  3.0),
        float2( 3.0, -1.0)
    };
    float2 uvs[3] = {
        float2(0.0, 1.0),
        float2(0.0, -1.0),
        float2(2.0, 1.0)
    };

    output.position = float4(positions[vertex_id], 0.0, 1.0);
    output.uv = uvs[vertex_id];
    return output;
}
)";

constexpr std::string_view kHdrCopyPixelShader = R"(
cbuffer Params : register(b0)
{
    float2 SourceOffset;
    float2 SourceScale;
    float Exposure;
    float ApplyToneMapping;
    float2 Padding;
};

Texture2D<float4> SourceTexture : register(t0);
SamplerState LinearClampSampler : register(s0);

struct VSOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(VSOut input) : SV_TARGET
{
    float2 sample_uv = input.uv * SourceScale + SourceOffset;
    return SourceTexture.Sample(LinearClampSampler, sample_uv);
}
)";

constexpr std::string_view kHdrTonemapPixelShader = R"(
cbuffer Params : register(b0)
{
    float2 SourceOffset;
    float2 SourceScale;
    float Exposure;
    float ApplyToneMapping;
    float2 Padding;
};

Texture2D<float4> SourceTexture : register(t0);
SamplerState LinearClampSampler : register(s0);

struct VSOut
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float tone_map_filmic(float value)
{
    value = max(value, 0.0);
    float numerator = value * (2.51 * value + 0.03);
    float denominator = value * (2.43 * value + 0.59) + 0.14;
    return denominator > 0.0 ? saturate(numerator / denominator) : 0.0;
}

float linear_to_srgb(float value)
{
    value = saturate(value);
    return value <= 0.0031308 ? value * 12.92 : 1.055 * pow(value, 1.0 / 2.4) - 0.055;
}

float4 main(VSOut input) : SV_TARGET
{
    float2 sample_uv = input.uv * SourceScale + SourceOffset;
    float3 rgb = max(SourceTexture.Sample(LinearClampSampler, sample_uv).rgb, 0.0.xxx);

    if (ApplyToneMapping > 0.5)
    {
        rgb *= Exposure;

        float luminance = dot(rgb, float3(0.2126, 0.7152, 0.0722));
        float mapped_luminance = tone_map_filmic(luminance);
        float scale = luminance > 1e-6 ? (mapped_luminance / luminance) : 0.0;
        rgb *= scale;
    }

    return float4(linear_to_srgb(rgb.r), linear_to_srgb(rgb.g), linear_to_srgb(rgb.b), 1.0);
}
)";

using D3DCompileProc = decltype(&D3DCompile);

D3DCompileProc get_d3d_compile_proc()
{
    static const auto loader = []() -> std::pair<HMODULE, D3DCompileProc> {
        constexpr std::array shader_compiler_dlls {
            L"d3dcompiler_47.dll",
            L"d3dcompiler_46.dll",
            L"d3dcompiler_43.dll",
        };

        for (const wchar_t* dll_name : shader_compiler_dlls) {
            HMODULE module = LoadLibraryW(dll_name);
            if (!module) {
                continue;
            }

            auto proc = reinterpret_cast<D3DCompileProc>(GetProcAddress(module, "D3DCompile"));
            if (!proc) {
                FreeLibrary(module);
                continue;
            }

            LogInfo << "Loaded D3D shader compiler dynamically" << VAR(dll_name);
            return { module, proc };
        }

        LogWarn << "No D3D shader compiler DLL found; HDR GPU preprocess will fall back to CPU";
        return { nullptr, nullptr };
    }();

    return loader.second;
}

bool get_window_frame_bounds(HWND hwnd, RECT& frame_rect)
{
    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }

    HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame_rect, sizeof(frame_rect));
    if (SUCCEEDED(hr)) {
        return true;
    }

    LogWarn << "DwmGetWindowAttribute failed, falling back to GetWindowRect" << VAR(hwnd) << VAR(hr);
    return GetWindowRect(hwnd, &frame_rect) != 0;
}

bool rect_has_area(const RECT& rect)
{
    return rect.right > rect.left && rect.bottom > rect.top;
}

bool is_texture_desc_compatible(const D3D11_TEXTURE2D_DESC& lhs, const D3D11_TEXTURE2D_DESC& rhs)
{
    return lhs.Width == rhs.Width && lhs.Height == rhs.Height && lhs.MipLevels == rhs.MipLevels && lhs.ArraySize == rhs.ArraySize
           && lhs.Format == rhs.Format && lhs.SampleDesc.Count == rhs.SampleDesc.Count && lhs.SampleDesc.Quality == rhs.SampleDesc.Quality;
}

struct OwnedWindowEnumContext
{
    HWND root = nullptr;
    DWORD process_id = 0;
    RECT bounds = { 0 };
    bool has_bounds = false;
};

BOOL CALLBACK enum_owned_windows_proc(HWND hwnd, LPARAM lparam)
{
    auto& context = *reinterpret_cast<OwnedWindowEnumContext*>(lparam);
    if (!hwnd || !IsWindow(hwnd) || !IsWindowVisible(hwnd) || IsIconic(hwnd)) {
        return TRUE;
    }

    DWORD process_id = 0;
    GetWindowThreadProcessId(hwnd, &process_id);
    if (process_id != context.process_id) {
        return TRUE;
    }

    HWND root = GetAncestor(hwnd, GA_ROOTOWNER);
    if (root != context.root) {
        return TRUE;
    }

    RECT frame_rect { 0 };
    if (!get_window_frame_bounds(hwnd, frame_rect)) {
        return TRUE;
    }

    if (!context.has_bounds) {
        context.bounds = frame_rect;
        context.has_bounds = true;
    }
    else {
        context.bounds.left = std::min(context.bounds.left, frame_rect.left);
        context.bounds.top = std::min(context.bounds.top, frame_rect.top);
        context.bounds.right = std::max(context.bounds.right, frame_rect.right);
        context.bounds.bottom = std::max(context.bounds.bottom, frame_rect.bottom);
    }

    return TRUE;
}

bool supports_include_secondary_windows()
{
    using namespace winrt::Windows::Foundation::Metadata;
    return ApiInformation::IsPropertyPresent(L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IncludeSecondaryWindows");
}

std::optional<RECT> get_capture_bounds(HWND hwnd, bool include_secondary_windows)
{
    RECT frame_rect { 0 };
    if (!get_window_frame_bounds(hwnd, frame_rect) || !rect_has_area(frame_rect)) {
        return std::nullopt;
    }

    RECT capture_bounds = frame_rect;
    if (!include_secondary_windows) {
        return capture_bounds;
    }

    HWND root = GetAncestor(hwnd, GA_ROOTOWNER);
    if (!root) {
        root = hwnd;
    }

    DWORD process_id = 0;
    GetWindowThreadProcessId(hwnd, &process_id);
    OwnedWindowEnumContext enum_context {
        .root = root,
        .process_id = process_id,
        .bounds = capture_bounds,
        .has_bounds = true,
    };

    EnumWindows(enum_owned_windows_proc, reinterpret_cast<LPARAM>(&enum_context));
    if (!enum_context.has_bounds || !rect_has_area(enum_context.bounds)) {
        return std::nullopt;
    }

    return enum_context.bounds;
}

struct CaptureMonitorEnumContext
{
    std::vector<HMONITOR>* monitors = nullptr;
};

BOOL CALLBACK enum_capture_monitors_proc(HMONITOR monitor, HDC, LPRECT, LPARAM lparam)
{
    auto& context = *reinterpret_cast<CaptureMonitorEnumContext*>(lparam);
    if (context.monitors && monitor) {
        context.monitors->emplace_back(monitor);
    }
    return TRUE;
}

std::vector<HMONITOR> get_capture_monitors(const RECT& capture_bounds)
{
    std::vector<HMONITOR> monitors;
    if (!rect_has_area(capture_bounds)) {
        return monitors;
    }

    CaptureMonitorEnumContext context { .monitors = &monitors };
    if (!EnumDisplayMonitors(nullptr, &capture_bounds, enum_capture_monitors_proc, reinterpret_cast<LPARAM>(&context))) {
        LogWarn << "EnumDisplayMonitors failed" << GetLastError();
    }

    if (monitors.empty()) {
        POINT center {
            capture_bounds.left + (capture_bounds.right - capture_bounds.left) / 2,
            capture_bounds.top + (capture_bounds.bottom - capture_bounds.top) / 2,
        };
        HMONITOR fallback_monitor = MonitorFromPoint(center, MONITOR_DEFAULTTONEAREST);
        if (fallback_monitor) {
            monitors.emplace_back(fallback_monitor);
        }
    }

    std::sort(monitors.begin(), monitors.end(), [](HMONITOR lhs, HMONITOR rhs) {
        return reinterpret_cast<std::uintptr_t>(lhs) < reinterpret_cast<std::uintptr_t>(rhs);
    });
    monitors.erase(std::unique(monitors.begin(), monitors.end()), monitors.end());
    return monitors;
}

std::optional<bool> is_windows_hdr_enabled(const std::vector<HMONITOR>& target_monitors)
{
    if (target_monitors.empty()) {
        LogDebug << "No capture monitors supplied; using global HDR query";
        return MAA_CTRL_UNIT_NS::query_windows_hdr_enabled_go_compatible();
    }

    bool has_valid_result = false;
    for (HMONITOR monitor : target_monitors) {
        const auto hdr_state = MAA_CTRL_UNIT_NS::query_hdr_display_state(monitor);
        if (!hdr_state.has_value() || !hdr_state->valid) {
            continue;
        }

        has_valid_result = true;
        if (hdr_state->hdr_enabled) {
            return true;
        }
    }

    if (has_valid_result) {
        return false;
    }

    LogWarn << "Per-monitor HDR query failed, falling back to global HDR query";
    return MAA_CTRL_UNIT_NS::query_windows_hdr_enabled_go_compatible();
}
} // namespace

MAA_CTRL_UNIT_NS_BEGIN

FramePoolScreencap::~FramePoolScreencap()
{
    uninit();
}

std::optional<cv::Mat> FramePoolScreencap::screencap()
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return std::nullopt;
    }
    if (!cap_frame_pool_) {
        if (!init()) {
            LogError << "init failed";
            uninit();
            return std::nullopt;
        }
    }

    // 检查窗口大小是否变化，如果变化则重新创建 frame pool
    if (!check_and_handle_size_changed()) {
        LogError << "check_and_handle_size_changed failed";
        return std::nullopt;
    }

    CaptureFrame frame = nullptr;

    try {
        // 优先消费 frame pool 里现成的最新帧；静态画面拿不到新帧时尽快回退到缓存，避免频繁卡满长超时。
        const auto timeout = cached_image_.empty() ? kColdFrameTimeout : kSteadyFrameTimeout;
        auto start_time = std::chrono::steady_clock::now();
        while (duration_since(start_time) < timeout) {
            while (auto next_frame = cap_frame_pool_.TryGetNextFrame()) {
                frame = std::move(next_frame);
            }
            if (frame) {
                break;
            }
            std::this_thread::sleep_for(kFramePollInterval);
        }
    }
    catch (const winrt::hresult_error& e) {
        LogError << "Failed to get frame" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
        uninit();
        return std::nullopt;
    }

    if (!frame) {
        if (cached_image_.empty()) {
            LogError << "Failed to get frame and no cached image available";
            return std::nullopt;
        }
        LogDebug << "No new frame available, reusing cached image";
        return cached_image_.clone();
    }

    auto surface = frame.Surface();
    if (!surface) {
        LogError << "frame.Surface() is null";
        return std::nullopt;
    }

    auto access = surface.try_as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    if (!access) {
        LogError << "Failed to get IDirect3DDxgiInterfaceAccess";
        return std::nullopt;
    }

    winrt::com_ptr<ID3D11Texture2D> texture = nullptr;
    HRESULT ret = access->GetInterface(winrt::guid_of<ID3D11Texture2D>(), texture.put_void());
    if (FAILED(ret)) {
        LogError << "GetInterface ID3D11Texture2D failed" << VAR(ret);
        return std::nullopt;
    }

    D3D11_TEXTURE2D_DESC current_desc { };
    texture->GetDesc(&current_desc);

    const int raw_width = static_cast<int>(current_desc.Width);
    const int raw_height = static_cast<int>(current_desc.Height);

    RECT client_rect = { 0 };
    if (!GetClientRect(hwnd_, &client_rect)) {
        LogError << "GetClientRect failed";
        return std::nullopt;
    }

    POINT client_top_left = { client_rect.left, client_rect.top };
    if (!ClientToScreen(hwnd_, &client_top_left)) {
        LogError << "ClientToScreen failed";
        return std::nullopt;
    }

    const auto capture_bounds_opt = get_capture_bounds(hwnd_, include_secondary_windows_enabled());
    if (!capture_bounds_opt.has_value()) {
        LogError << "Failed to get target capture bounds";
        return std::nullopt;
    }
    const RECT capture_bounds = *capture_bounds_opt;

    int border_left = client_top_left.x - capture_bounds.left;
    int border_top = client_top_left.y - capture_bounds.top;

    int client_width = client_rect.right - client_rect.left;
    int client_height = client_rect.bottom - client_rect.top;

    if (border_left < 0) {
        border_left = 0;
    }
    if (border_top < 0) {
        border_top = 0;
    }
    if (client_width > raw_width) {
        client_width = raw_width;
    }
    if (border_left + client_width > raw_width) {
        border_left = raw_width - client_width;
    }
    if (client_height > raw_height) {
        client_height = raw_height;
    }
    if (border_top + client_height > raw_height) {
        border_top = raw_height - client_height;
    }

    cv::Rect client_roi(border_left, border_top, client_width, client_height);

    if (current_desc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT) {
        if (cv::Mat gpu_processed; try_process_hdr_on_gpu(texture, client_roi, gpu_processed)) {
            last_screencap_info_ = {
                .hdr_capture_active = true,
                .hdr_preprocessed = true,
                .gpu_processed = true,
                .display_hdr_active = true,
                .display_hdr_compensated = false,
            };
            cached_image_ = std::move(gpu_processed);
            return cached_image_;
        }
    }

    if (!readable_texture_ || !is_texture_desc_compatible(current_desc, texture_desc_)) {
        if (readable_texture_) {
            LogInfo << "Capture frame texture changed, recreating staging texture"
                    << VAR(texture_desc_.Width) << VAR(texture_desc_.Height) << VAR(texture_desc_.Format)
                    << VAR(current_desc.Width) << VAR(current_desc.Height) << VAR(current_desc.Format);
        }
        readable_texture_ = nullptr;
        texture_desc_ = { 0 };
        if (!init_texture(texture)) {
            LogError << "falied to init_texture";
            return std::nullopt;
        }
    }

    d3d_context_->CopyResource(readable_texture_.get(), texture.get());

    D3D11_MAPPED_SUBRESOURCE mapped { 0 };
    ret = d3d_context_->Map(readable_texture_.get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(ret)) {
        LogError << "Map failed" << VAR(ret);
        return std::nullopt;
    }
    OnScopeLeave([&]() { d3d_context_->Unmap(readable_texture_.get(), 0); });

    switch (current_desc.Format) {
    case DXGI_FORMAT_B8G8R8A8_UNORM: {
        cv::Mat raw(raw_height, raw_width, CV_8UC4, mapped.pData, mapped.RowPitch);
        last_screencap_info_ = {};
        cached_image_ = bgra_to_bgr(raw(client_roi));
        return cached_image_;
    }
    case DXGI_FORMAT_R16G16B16A16_FLOAT: {
        cv::Mat raw(raw_height, raw_width, CV_16FC4, mapped.pData, mapped.RowPitch);
        last_screencap_info_ = {
            .hdr_capture_active = true,
            .hdr_preprocessed = false,
            .gpu_processed = false,
            .display_hdr_active = true,
            .display_hdr_compensated = false,
        };
        cached_image_ = raw(client_roi).clone();
        return cached_image_;
    }
    default:
        LogError << "Unsupported frame format" << VAR(texture_desc_.Format);
        return std::nullopt;
    }
}

bool FramePoolScreencap::init()
{
    LogFunc;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    HRESULT ret = S_OK;

    DXGI_SWAP_CHAIN_DESC swap_chain_desc = { };
    swap_chain_desc.BufferCount = 1;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.OutputWindow = hwnd_;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.Windowed = TRUE;

    ret = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &swap_chain_desc,
        dxgi_swap_chain_.put(),
        d3d_device_.put(),
        nullptr,
        d3d_context_.put());

    if (FAILED(ret)) {
        LogError << "D3D11CreateDeviceAndSwapChain failed" << VAR(ret);
        return false;
    }

    try {
        auto activation_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
        auto interop_factory = activation_factory.try_as<IGraphicsCaptureItemInterop>();
        if (!interop_factory) {
            LogError << "Failed to get IGraphicsCaptureItemInterop";
            return false;
        }
        ret = interop_factory->CreateForWindow(
            hwnd_,
            winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
            winrt::put_abi(cap_item_));
    }
    catch (const winrt::hresult_error& e) {
        LogError << "Failed to create GraphicsCaptureItem" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
        return false;
    }
    if (FAILED(ret)) {
        LogError << "CreateForWindow GraphicsCaptureItem failed" << VAR(ret);
        return false;
    }

    if (!cap_item_) {
        LogError << "cap_item_ is null after CreateForWindow";
        return false;
    }

    auto item_size = cap_item_.Size();
    if (item_size.Width <= 0 || item_size.Height <= 0) {
        LogError << "Invalid capture item size" << VAR(item_size.Width) << VAR(item_size.Height);
        return false;
    }

    if (!IsWindow(hwnd_) || !IsWindowVisible(hwnd_)) {
        LogError << "Window is no longer valid or visible";
        return false;
    }

    auto dxgi_device = d3d_device_.try_as<IDXGIDevice>();
    if (!dxgi_device) {
        LogError << "Failed to get IDXGIDevice";
        return false;
    }

    winrt::com_ptr<IInspectable> inspectable = nullptr;
    ret = CreateDirect3D11DeviceFromDXGIDevice(dxgi_device.get(), inspectable.put());
    if (FAILED(ret)) {
        LogError << "CreateDirect3D11DeviceFromDXGIDevice failed" << VAR(ret);
        return false;
    }

    auto d3d_device_interop = inspectable.try_as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
    if (!d3d_device_interop) {
        LogError << "Failed to get IDirect3DDevice";
        return false;
    }

    const HMONITOR current_monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
    if (!current_monitor) {
        LogWarn << "MonitorFromWindow failed, falling back to HDR-first probing";
    }

    const bool try_secondary_windows = supports_include_secondary_windows();
    std::vector<HMONITOR> capture_monitors;
    if (const auto capture_bounds = get_capture_bounds(hwnd_, try_secondary_windows)) {
        capture_monitors = get_capture_monitors(*capture_bounds);
    }
    if (capture_monitors.empty() && current_monitor) {
        capture_monitors.emplace_back(current_monitor);
    }

    if (!capture_monitors.empty()) {
        LogInfo << "Capture intersects monitors" << VAR(capture_monitors.size());
    }

    const std::optional<bool> hdr_enabled = is_windows_hdr_enabled(capture_monitors);
    const bool prefer_hdr = hdr_enabled.value_or(true);

    if (!hdr_enabled.has_value()) {
        LogWarn << "Advanced-color display detection failed, keeping HDR-first probing";
    }
    else {
        LogInfo << "Advanced-color display detection result" << VAR(hdr_enabled.value());
    }

    if (prefer_hdr) {
        LogInfo << "Attempting float frame pool for advanced-color display";
        if (!try_init_frame_pool(d3d_device_interop, winrt::Windows::Graphics::DirectX::DirectXPixelFormat::R16G16B16A16Float)) {
            LogWarn << "Float frame pool unavailable, falling back to SDR";
            if (!try_init_frame_pool(d3d_device_interop, winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized)) {
                return false;
            }
        }
    }
    else {
        LogInfo << "Advanced-color display is off, using SDR frame pool";
        if (!try_init_frame_pool(d3d_device_interop, winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized)) {
            return false;
        }
    }

    try {
        cap_session_ = cap_frame_pool_.CreateCaptureSession(cap_item_);
    }
    catch (const winrt::hresult_error& e) {
        LogError << "CreateCaptureSession failed" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
        return false;
    }
    if (!cap_session_) {
        LogError << "CreateCaptureSession returned null";
        return false;
    }

    // Try to disable the yellow capture border when the OS supports it.
    try_disable_border();

    // 尝试关闭截图时的鼠标指针（Windows 10 2004 及以上支持）
    try_disable_cursor();

    // Try to include owned popups such as dialogs or tooltips in the capture.
    try_include_secondary_windows();

    try {
        cap_session_.StartCapture();
    }
    catch (const winrt::hresult_error& e) {
        LogError << "StartCapture failed" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
        return false;
    }

    // 记录初始窗口大小
    if (cap_item_) {
        auto size = cap_item_.Size();
        last_capture_size_.first = size.Width;
        last_capture_size_.second = size.Height;
    }
    last_monitor_ = current_monitor;
    last_capture_monitors_ = std::move(capture_monitors);
    last_capture_prefers_hdr_ = prefer_hdr;

    return true;
}

void FramePoolScreencap::uninit()
{
    if (cap_session_) {
        try {
            cap_session_.Close();
        }
        catch (const winrt::hresult_error& e) {
            LogWarn << "cap_session_.Close() failed" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
        }
        cap_session_ = nullptr;
    }

    if (cap_frame_pool_) {
        try {
            cap_frame_pool_.Close();
        }
        catch (const winrt::hresult_error& e) {
            LogWarn << "cap_frame_pool_.Close() failed" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
        }
        cap_frame_pool_ = nullptr;
    }

    readable_texture_ = nullptr;
    reset_hdr_gpu_resources();
    hdr_vertex_shader_ = nullptr;
    hdr_copy_pixel_shader_ = nullptr;
    hdr_tonemap_pixel_shader_ = nullptr;
    hdr_sampler_state_ = nullptr;
    hdr_constant_buffer_ = nullptr;
    hdr_gpu_pipeline_attempted_ = false;
    hdr_gpu_pipeline_available_ = false;
    cap_item_ = nullptr;
    cap_pixel_format_ = winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized;
    texture_desc_ = { 0 };
    last_monitor_ = nullptr;
    last_capture_monitors_.clear();
    last_capture_prefers_hdr_ = true;
    last_capture_size_ = { };
    cached_image_ = cv::Mat();
    include_secondary_windows_enabled_ = false;
    last_screencap_info_ = {};
}

bool FramePoolScreencap::check_and_handle_size_changed()
{
    if (!cap_item_) {
        return true;
    }

    if (!IsWindow(hwnd_)) {
        LogError << "Window is no longer valid";
        return false;
    }

    const HMONITOR current_monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
    const bool primary_monitor_changed = current_monitor && last_monitor_ && current_monitor != last_monitor_;

    std::vector<HMONITOR> current_capture_monitors;
    if (const auto capture_bounds = get_capture_bounds(hwnd_, include_secondary_windows_enabled())) {
        current_capture_monitors = get_capture_monitors(*capture_bounds);
    }
    if (current_capture_monitors.empty() && current_monitor) {
        current_capture_monitors.emplace_back(current_monitor);
    }

    const bool capture_monitors_changed = current_capture_monitors != last_capture_monitors_;
    const std::optional<bool> hdr_enabled = is_windows_hdr_enabled(current_capture_monitors);
    const bool prefer_hdr = hdr_enabled.value_or(true);
    const bool hdr_preference_changed = prefer_hdr != last_capture_prefers_hdr_;

    winrt::Windows::Graphics::SizeInt32 current_size { };
    try {
        current_size = cap_item_.Size();
    }
    catch (const winrt::hresult_error& e) {
        LogError << "Failed to get capture item size" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
        uninit();
        return false;
    }
    if (!primary_monitor_changed && !capture_monitors_changed && !hdr_preference_changed && current_size.Width == last_capture_size_.first
        && current_size.Height == last_capture_size_.second) {
        return true;
    }

    if (primary_monitor_changed) {
        LogInfo << "Window monitor changed, recreating frame pool" << VAR_VOIDP(last_monitor_) << VAR_VOIDP(current_monitor)
                << VAR(current_size.Width) << VAR(current_size.Height);
    }
    else if (capture_monitors_changed) {
        LogInfo << "Capture monitor set changed, recreating frame pool" << VAR(last_capture_monitors_.size())
                << VAR(current_capture_monitors.size()) << VAR(current_size.Width) << VAR(current_size.Height);
    }
    else if (hdr_preference_changed) {
        LogInfo << "Capture HDR preference changed, recreating frame pool" << VAR(last_capture_prefers_hdr_) << VAR(prefer_hdr)
                << VAR(current_size.Width) << VAR(current_size.Height);
    }
    else {
        LogInfo << "Window size changed, recreating frame pool" << VAR(current_size.Width) << VAR(current_size.Height)
                << VAR(last_capture_size_.first) << VAR(last_capture_size_.second);
    }

    // 完全重新初始化以适应新的窗口大小
    uninit();
    if (!init()) {
        LogError << "reinit failed after size change";
        return false;
    }

    return true;
}

bool FramePoolScreencap::init_texture(winrt::com_ptr<ID3D11Texture2D> raw_texture)
{
    LogFunc;

    if (!d3d_device_ || !raw_texture) {
        LogError << "handle is null";
        return false;
    }

    raw_texture->GetDesc(&texture_desc_);

    texture_desc_.BindFlags = 0;
    texture_desc_.MiscFlags = 0;
    texture_desc_.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    texture_desc_.Usage = D3D11_USAGE_STAGING;

    HRESULT ret = d3d_device_->CreateTexture2D(&texture_desc_, NULL, readable_texture_.put());
    if (FAILED(ret)) {
        LogError << "CreateTexture2D failed" << VAR(ret);
        return false;
    }

    return true;
}

bool FramePoolScreencap::init_hdr_gpu_pipeline()
{
    if (hdr_gpu_pipeline_attempted_) {
        return hdr_gpu_pipeline_available_;
    }
    hdr_gpu_pipeline_attempted_ = true;

    if (!d3d_device_) {
        LogError << "d3d_device_ is null";
        return false;
    }

    const auto compile_shader = [](std::string_view source, const char* entry, const char* target, winrt::com_ptr<ID3DBlob>& bytecode) {
        const auto d3d_compile = get_d3d_compile_proc();
        if (!d3d_compile) {
            LogWarn << "D3DCompile is unavailable, skip HDR GPU preprocess shader compilation";
            return false;
        }

        winrt::com_ptr<ID3DBlob> errors;
        winrt::com_ptr<ID3DBlob> compiled;

        const HRESULT ret = d3d_compile(
            source.data(),
            source.size(),
            nullptr,
            nullptr,
            nullptr,
            entry,
            target,
            D3DCOMPILE_ENABLE_STRICTNESS,
            0,
            compiled.put(),
            errors.put());
        if (FAILED(ret)) {
            const std::string error_text = errors ? std::string(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize()) : "";
            LogWarn << "D3DCompile failed" << VAR(entry) << VAR(target) << VAR(ret) << VAR(error_text);
            return false;
        }

        bytecode = std::move(compiled);
        return true;
    };

    winrt::com_ptr<ID3DBlob> vertex_shader_blob;
    if (!compile_shader(kHdrFullscreenVertexShader, "main", "vs_4_0", vertex_shader_blob)) {
        return false;
    }

    HRESULT ret = d3d_device_->CreateVertexShader(
        vertex_shader_blob->GetBufferPointer(),
        vertex_shader_blob->GetBufferSize(),
        nullptr,
        hdr_vertex_shader_.put());
    if (FAILED(ret)) {
        LogWarn << "CreateVertexShader failed" << VAR(ret);
        return false;
    }

    winrt::com_ptr<ID3DBlob> copy_pixel_shader_blob;
    if (!compile_shader(kHdrCopyPixelShader, "main", "ps_4_0", copy_pixel_shader_blob)) {
        return false;
    }
    ret = d3d_device_->CreatePixelShader(
        copy_pixel_shader_blob->GetBufferPointer(),
        copy_pixel_shader_blob->GetBufferSize(),
        nullptr,
        hdr_copy_pixel_shader_.put());
    if (FAILED(ret)) {
        LogWarn << "Create copy pixel shader failed" << VAR(ret);
        return false;
    }

    winrt::com_ptr<ID3DBlob> tonemap_pixel_shader_blob;
    if (!compile_shader(kHdrTonemapPixelShader, "main", "ps_4_0", tonemap_pixel_shader_blob)) {
        return false;
    }
    ret = d3d_device_->CreatePixelShader(
        tonemap_pixel_shader_blob->GetBufferPointer(),
        tonemap_pixel_shader_blob->GetBufferSize(),
        nullptr,
        hdr_tonemap_pixel_shader_.put());
    if (FAILED(ret)) {
        LogWarn << "Create tonemap pixel shader failed" << VAR(ret);
        return false;
    }

    D3D11_SAMPLER_DESC sampler_desc {};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampler_desc.MinLOD = 0.0f;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

    ret = d3d_device_->CreateSamplerState(&sampler_desc, hdr_sampler_state_.put());
    if (FAILED(ret)) {
        LogWarn << "CreateSamplerState failed" << VAR(ret);
        return false;
    }

    D3D11_BUFFER_DESC constant_buffer_desc {};
    constant_buffer_desc.ByteWidth = sizeof(HdrGpuShaderConstants);
    constant_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    ret = d3d_device_->CreateBuffer(&constant_buffer_desc, nullptr, hdr_constant_buffer_.put());
    if (FAILED(ret)) {
        LogWarn << "CreateBuffer failed" << VAR(ret);
        return false;
    }

    hdr_gpu_pipeline_available_ = true;
    LogInfo << "HDR GPU preprocess pipeline initialized";
    return true;
}

void FramePoolScreencap::reset_hdr_gpu_resources()
{
    hdr_gpu_source_desc_ = {};
    hdr_gpu_output_size_ = {};
    hdr_gpu_stats_size_ = {};
    hdr_shader_input_texture_ = nullptr;
    hdr_shader_input_srv_ = nullptr;
    hdr_stats_texture_ = nullptr;
    hdr_stats_rtv_ = nullptr;
    hdr_stats_staging_ = nullptr;
    hdr_output_texture_ = nullptr;
    hdr_output_rtv_ = nullptr;
    hdr_output_staging_ = nullptr;
}

bool FramePoolScreencap::ensure_hdr_gpu_resources(const D3D11_TEXTURE2D_DESC& source_desc, const cv::Size& output_size)
{
    if (!hdr_gpu_pipeline_available_ && !init_hdr_gpu_pipeline()) {
        return false;
    }

    if (output_size.width <= 0 || output_size.height <= 0) {
        LogWarn << "Invalid HDR GPU output size" << VAR(output_size.width) << VAR(output_size.height);
        return false;
    }

    const cv::Size stats_size = calc_hdr_stats_size(output_size.width, output_size.height);
    if (stats_size.width <= 0 || stats_size.height <= 0) {
        LogWarn << "Invalid HDR stats texture size" << VAR(stats_size.width) << VAR(stats_size.height);
        return false;
    }

    if (hdr_shader_input_texture_ && is_texture_desc_compatible(hdr_gpu_source_desc_, source_desc) && hdr_gpu_output_size_ == output_size
        && hdr_gpu_stats_size_ == stats_size) {
        return true;
    }

    reset_hdr_gpu_resources();

    D3D11_TEXTURE2D_DESC shader_input_desc = source_desc;
    shader_input_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    shader_input_desc.MiscFlags = 0;
    shader_input_desc.CPUAccessFlags = 0;
    shader_input_desc.Usage = D3D11_USAGE_DEFAULT;

    HRESULT ret = d3d_device_->CreateTexture2D(&shader_input_desc, nullptr, hdr_shader_input_texture_.put());
    if (FAILED(ret)) {
        LogWarn << "Create shader input texture failed" << VAR(ret);
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc {};
    srv_desc.Format = shader_input_desc.Format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;

    ret = d3d_device_->CreateShaderResourceView(hdr_shader_input_texture_.get(), &srv_desc, hdr_shader_input_srv_.put());
    if (FAILED(ret)) {
        LogWarn << "Create shader resource view failed" << VAR(ret);
        return false;
    }

    D3D11_TEXTURE2D_DESC stats_texture_desc {};
    stats_texture_desc.Width = static_cast<UINT>(stats_size.width);
    stats_texture_desc.Height = static_cast<UINT>(stats_size.height);
    stats_texture_desc.MipLevels = 1;
    stats_texture_desc.ArraySize = 1;
    stats_texture_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    stats_texture_desc.SampleDesc.Count = 1;
    stats_texture_desc.Usage = D3D11_USAGE_DEFAULT;
    stats_texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET;

    ret = d3d_device_->CreateTexture2D(&stats_texture_desc, nullptr, hdr_stats_texture_.put());
    if (FAILED(ret)) {
        LogWarn << "Create HDR stats texture failed" << VAR(ret);
        return false;
    }

    ret = d3d_device_->CreateRenderTargetView(hdr_stats_texture_.get(), nullptr, hdr_stats_rtv_.put());
    if (FAILED(ret)) {
        LogWarn << "Create HDR stats RTV failed" << VAR(ret);
        return false;
    }

    D3D11_TEXTURE2D_DESC stats_staging_desc = stats_texture_desc;
    stats_staging_desc.BindFlags = 0;
    stats_staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stats_staging_desc.Usage = D3D11_USAGE_STAGING;

    ret = d3d_device_->CreateTexture2D(&stats_staging_desc, nullptr, hdr_stats_staging_.put());
    if (FAILED(ret)) {
        LogWarn << "Create HDR stats staging texture failed" << VAR(ret);
        return false;
    }

    D3D11_TEXTURE2D_DESC output_texture_desc {};
    output_texture_desc.Width = static_cast<UINT>(output_size.width);
    output_texture_desc.Height = static_cast<UINT>(output_size.height);
    output_texture_desc.MipLevels = 1;
    output_texture_desc.ArraySize = 1;
    output_texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    output_texture_desc.SampleDesc.Count = 1;
    output_texture_desc.Usage = D3D11_USAGE_DEFAULT;
    output_texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET;

    ret = d3d_device_->CreateTexture2D(&output_texture_desc, nullptr, hdr_output_texture_.put());
    if (FAILED(ret)) {
        LogWarn << "Create HDR output texture failed" << VAR(ret);
        return false;
    }

    ret = d3d_device_->CreateRenderTargetView(hdr_output_texture_.get(), nullptr, hdr_output_rtv_.put());
    if (FAILED(ret)) {
        LogWarn << "Create HDR output RTV failed" << VAR(ret);
        return false;
    }

    D3D11_TEXTURE2D_DESC output_staging_desc = output_texture_desc;
    output_staging_desc.BindFlags = 0;
    output_staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    output_staging_desc.Usage = D3D11_USAGE_STAGING;

    ret = d3d_device_->CreateTexture2D(&output_staging_desc, nullptr, hdr_output_staging_.put());
    if (FAILED(ret)) {
        LogWarn << "Create HDR output staging texture failed" << VAR(ret);
        return false;
    }

    hdr_gpu_source_desc_ = source_desc;
    hdr_gpu_output_size_ = output_size;
    hdr_gpu_stats_size_ = stats_size;
    return true;
}

bool FramePoolScreencap::try_process_hdr_on_gpu(const winrt::com_ptr<ID3D11Texture2D>& texture, const cv::Rect& client_roi, cv::Mat& output)
{
    if (!texture || !d3d_context_) {
        return false;
    }

    D3D11_TEXTURE2D_DESC source_desc {};
    texture->GetDesc(&source_desc);

    if (!ensure_hdr_gpu_resources(source_desc, client_roi.size())) {
        return false;
    }

    d3d_context_->CopyResource(hdr_shader_input_texture_.get(), texture.get());

    const HdrGpuShaderConstants constants {
        .source_offset = { static_cast<float>(client_roi.x) / source_desc.Width, static_cast<float>(client_roi.y) / source_desc.Height },
        .source_scale = { static_cast<float>(client_roi.width) / source_desc.Width, static_cast<float>(client_roi.height) / source_desc.Height },
        .exposure = 1.0f,
        .apply_tone_mapping = 0.0f,
    };
    d3d_context_->UpdateSubresource(hdr_constant_buffer_.get(), 0, nullptr, &constants, 0, 0);

    ID3D11Buffer* constant_buffers[] = { hdr_constant_buffer_.get() };
    ID3D11ShaderResourceView* shader_resources[] = { hdr_shader_input_srv_.get() };
    ID3D11SamplerState* samplers[] = { hdr_sampler_state_.get() };

    d3d_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    d3d_context_->IASetInputLayout(nullptr);
    d3d_context_->VSSetShader(hdr_vertex_shader_.get(), nullptr, 0);
    d3d_context_->VSSetConstantBuffers(0, 1, constant_buffers);
    d3d_context_->PSSetShaderResources(0, 1, shader_resources);
    d3d_context_->PSSetSamplers(0, 1, samplers);
    d3d_context_->PSSetConstantBuffers(0, 1, constant_buffers);

    D3D11_VIEWPORT stats_viewport {};
    stats_viewport.Width = static_cast<float>(hdr_gpu_stats_size_.width);
    stats_viewport.Height = static_cast<float>(hdr_gpu_stats_size_.height);
    stats_viewport.MinDepth = 0.0f;
    stats_viewport.MaxDepth = 1.0f;

    ID3D11RenderTargetView* stats_rtv[] = { hdr_stats_rtv_.get() };
    d3d_context_->RSSetViewports(1, &stats_viewport);
    d3d_context_->OMSetRenderTargets(1, stats_rtv, nullptr);
    d3d_context_->PSSetShader(hdr_copy_pixel_shader_.get(), nullptr, 0);
    d3d_context_->Draw(3, 0);
    d3d_context_->CopyResource(hdr_stats_staging_.get(), hdr_stats_texture_.get());

    D3D11_MAPPED_SUBRESOURCE mapped_stats {};
    HRESULT ret = d3d_context_->Map(hdr_stats_staging_.get(), 0, D3D11_MAP_READ, 0, &mapped_stats);
    if (FAILED(ret)) {
        LogWarn << "Map HDR stats staging texture failed" << VAR(ret);
        return false;
    }
    OnScopeLeave([&]() { d3d_context_->Unmap(hdr_stats_staging_.get(), 0); });

    cv::Mat hdr_stats_mat(hdr_gpu_stats_size_.height, hdr_gpu_stats_size_.width, CV_16FC4, mapped_stats.pData, mapped_stats.RowPitch);
    const HdrSceneStats hdr_scene_stats = analyze_hdr_scene(hdr_stats_mat);
    bool apply_tone_mapping = false;
    const float exposure = calc_hdr_exposure(hdr_scene_stats, apply_tone_mapping);

    HdrGpuShaderConstants tonemap_constants {
        .source_offset = { static_cast<float>(client_roi.x) / source_desc.Width, static_cast<float>(client_roi.y) / source_desc.Height },
        .source_scale = { static_cast<float>(client_roi.width) / source_desc.Width, static_cast<float>(client_roi.height) / source_desc.Height },
        .exposure = exposure,
        .apply_tone_mapping = apply_tone_mapping ? 1.0f : 0.0f,
    };
    d3d_context_->UpdateSubresource(hdr_constant_buffer_.get(), 0, nullptr, &tonemap_constants, 0, 0);

    D3D11_VIEWPORT output_viewport {};
    output_viewport.Width = static_cast<float>(hdr_gpu_output_size_.width);
    output_viewport.Height = static_cast<float>(hdr_gpu_output_size_.height);
    output_viewport.MinDepth = 0.0f;
    output_viewport.MaxDepth = 1.0f;

    ID3D11RenderTargetView* output_rtv[] = { hdr_output_rtv_.get() };
    d3d_context_->RSSetViewports(1, &output_viewport);
    d3d_context_->OMSetRenderTargets(1, output_rtv, nullptr);
    d3d_context_->PSSetShader(hdr_tonemap_pixel_shader_.get(), nullptr, 0);
    d3d_context_->Draw(3, 0);

    ID3D11ShaderResourceView* null_shader_resources[] = { nullptr };
    d3d_context_->PSSetShaderResources(0, 1, null_shader_resources);

    d3d_context_->CopyResource(hdr_output_staging_.get(), hdr_output_texture_.get());

    D3D11_MAPPED_SUBRESOURCE mapped_output {};
    ret = d3d_context_->Map(hdr_output_staging_.get(), 0, D3D11_MAP_READ, 0, &mapped_output);
    if (FAILED(ret)) {
        LogWarn << "Map HDR output staging texture failed" << VAR(ret);
        return false;
    }
    OnScopeLeave([&]() { d3d_context_->Unmap(hdr_output_staging_.get(), 0); });

    cv::Mat gpu_bgra(hdr_gpu_output_size_.height, hdr_gpu_output_size_.width, CV_8UC4, mapped_output.pData, mapped_output.RowPitch);
    output = bgra_to_bgr(gpu_bgra);
    return !output.empty();
}

bool FramePoolScreencap::try_init_frame_pool(
    const winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice& d3d_device_interop,
    winrt::Windows::Graphics::DirectX::DirectXPixelFormat pixel_format)
{
    try {
        cap_frame_pool_ = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::Create(
            d3d_device_interop,
            pixel_format,
            1,
            cap_item_.Size());
    }
    catch (const winrt::hresult_error& e) {
        LogWarn << "Direct3D11CaptureFramePool::Create failed" << VAR(pixel_format) << VAR(e.code())
                << VAR(winrt::to_string(e.message()));
        cap_frame_pool_ = nullptr;
        return false;
    }

    if (!cap_frame_pool_) {
        LogWarn << "Direct3D11CaptureFramePool::Create returned null" << VAR(pixel_format);
        return false;
    }

    cap_pixel_format_ = pixel_format;
    LogInfo << "Frame pool initialized" << VAR(cap_pixel_format_);
    return true;
}

void FramePoolScreencap::try_disable_border()
{
    LogFunc;

    using namespace winrt::Windows::Foundation::Metadata;
    using namespace winrt::Windows::Graphics::Capture;

    // GraphicsCaptureAccess and IsBorderRequired were introduced in
    // UniversalApiContract v10.0 (Windows 10 2004).
    if (!ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 10)) {
        LogInfo << "UniversalApiContract v10 not present, border toggle not supported";
        return;
    }

    if (!ApiInformation::IsTypePresent(L"Windows.Graphics.Capture.GraphicsCaptureAccess")) {
        LogInfo << "GraphicsCaptureAccess not present, border toggle not supported";
        return;
    }

    if (!ApiInformation::IsPropertyPresent(L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IsBorderRequired")) {
        LogInfo << "IsBorderRequired property not supported on this system";
        return;
    }

    try {
        auto op = GraphicsCaptureAccess::RequestAccessAsync(GraphicsCaptureAccessKind::Borderless);
        auto status = op.wait_for(std::chrono::seconds(5));
        if (status != winrt::Windows::Foundation::AsyncStatus::Completed) {
            LogWarn << "RequestAccessAsync did not complete in time";
            return;
        }

        auto access_result = op.GetResults();
        if (access_result != winrt::Windows::Security::Authorization::AppCapabilityAccess::AppCapabilityAccessStatus::Allowed) {
            LogWarn << "Borderless capture access not granted:" << static_cast<int>(access_result);
            return;
        }

        cap_session_.IsBorderRequired(false);
        LogInfo << "Capture border disabled successfully";
    }
    catch (const winrt::hresult_error& e) {
        LogWarn << "Failed to disable capture border" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
    }
}

void FramePoolScreencap::try_disable_cursor()
{
    LogFunc;

    using namespace winrt::Windows::Foundation::Metadata;

    if (!ApiInformation::IsPropertyPresent(L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IsCursorCaptureEnabled")) {
        LogInfo << "IsCursorCaptureEnabled property not supported on this system";
        return;
    }

    try {
        cap_session_.IsCursorCaptureEnabled(false);
        LogInfo << "Cursor capture disabled successfully";
    }
    catch (const winrt::hresult_error& e) {
        LogWarn << "Failed to disable cursor capture" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
    }
}

void FramePoolScreencap::try_include_secondary_windows()
{
    LogFunc;

    using namespace winrt::Windows::Foundation::Metadata;

    if (!ApiInformation::IsPropertyPresent(L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IncludeSecondaryWindows")) {
        LogInfo << "IncludeSecondaryWindows property not supported on this system";
        return;
    }

    try {
        cap_session_.IncludeSecondaryWindows(true);
        include_secondary_windows_enabled_ = true;
        LogInfo << "Secondary windows capture enabled successfully";
    }
    catch (const winrt::hresult_error& e) {
        LogWarn << "Failed to enable secondary windows capture" << VAR(e.code()) << VAR(winrt::to_string(e.message()));
    }
}

MAA_CTRL_UNIT_NS_END

#endif
