#pragma once

#include <algorithm>
#include <cmath>
#include <utility>

#include "Common/Conf.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "MaaUtils/SafeWindows.hpp"
#include "SafeDXGI.hpp"

// DirectXMath for accurate half float conversion
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

MAA_CTRL_UNIT_NS_BEGIN

inline double window_scale(HWND hwnd)
{
#ifndef MAA_WIN32_COMPATIBLE

    constexpr double kStandardDPI = 96.0;
    // 运行期需要 Win10 1607 以上版本
    return GetDpiForWindow(hwnd) / kStandardDPI;

#else

    HMONITOR monitor_handle = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

    MONITORINFOEX miex { 0 };
    miex.cbSize = sizeof(miex);
    GetMonitorInfo(monitor_handle, &miex);
    LONG screen_x_logical = miex.rcMonitor.right - miex.rcMonitor.left;

    DEVMODE dm { 0 };
    dm.dmSize = sizeof(dm);
    EnumDisplaySettings(miex.szDevice, ENUM_CURRENT_SETTINGS, &dm);
    DWORD screen_x_physical = dm.dmPelsWidth;

    return static_cast<double>(screen_x_physical) / static_cast<double>(screen_x_logical);

#endif
}

inline std::pair<int, int> window_size(HWND hwnd)
{
    RECT rect { 0 };
    if (!GetClientRect(hwnd, &rect)) {
        return { 0, 0 };
    }

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    return { width, height };
}

inline bool is_fullscreen(HWND hwnd)
{
    return GetWindowLongPtr(hwnd, GWL_STYLE) & WS_POPUP;
}

inline bool is_hdr_format(DXGI_FORMAT format)
{
    switch (format) {
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return true;
    default:
        return false;
    }
}

// 使用 DirectXMath 将 half float (16-bit) 转换为 float
inline float half_to_float(uint16_t h)
{
    return DirectX::PackedVector::XMConvertHalfToFloat(h);
}

// ACES Filmic Tone Mapping
// 这个曲线在 SDR 范围内接近线性，只压缩 HDR 高光
// 参考: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
inline float aces_filmic_tonemap(float x)
{
    constexpr float a = 2.51f;
    constexpr float b = 0.03f;
    constexpr float c = 2.43f;
    constexpr float d = 0.59f;
    constexpr float e = 0.14f;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

// scRGB/HDR 线性值转换为 sRGB gamma
inline float linear_to_srgb_gamma(float linear)
{
    if (linear <= 0.0031308f) {
        return linear * 12.92f;
    }
    return 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
}

// 将 R16G16B16A16_FLOAT HDR (scRGB) 图像转换为 SDR BGRA 图像
// scRGB 色彩空间：1.0 = 80 nits（参考白点）
// sdr_white_level：Windows HDR 模式下 SDR 内容的白点亮度（nits），默认 200
inline cv::Mat hdr_float16_to_sdr_bgra(const void* data, int width, int height, int row_pitch, float sdr_white_level = 200.0f)
{
    cv::Mat result(height, width, CV_8UC4);

    const uint8_t* src_row = static_cast<const uint8_t*>(data);

    // scRGB 中 1.0 = 80 nits
    // Windows HDR 模式下，SDR 白点通常设置为 200 nits（可在系统设置中调整）
    // 需要将 scRGB 值除以 (sdr_white_level / 80) 来归一化
    constexpr float scrgb_white_nits = 80.0f;
    const float normalization_factor = scrgb_white_nits / sdr_white_level;

    for (int y = 0; y < height; ++y) {
        const uint16_t* src = reinterpret_cast<const uint16_t*>(src_row);
        cv::Vec4b* dst = result.ptr<cv::Vec4b>(y);

        for (int x = 0; x < width; ++x) {
            // R16G16B16A16_FLOAT: 每个通道 16 bit，顺序为 RGBA
            float r = half_to_float(src[0]);
            float g = half_to_float(src[1]);
            float b = half_to_float(src[2]);
            float a = half_to_float(src[3]);

            // 处理负值（scRGB 允许负值表示超出 sRGB 色域的颜色）
            r = std::max(0.0f, r);
            g = std::max(0.0f, g);
            b = std::max(0.0f, b);

            // 根据 SDR 白点归一化：将 SDR 范围（0 到 sdr_white_level nits）映射到 0-1
            r *= normalization_factor;
            g *= normalization_factor;
            b *= normalization_factor;

            // 对超出 SDR 范围的 HDR 高光应用 ACES Filmic tone mapping
            // ACES 曲线在 SDR 范围内接近线性，只压缩高光
            r = aces_filmic_tonemap(r);
            g = aces_filmic_tonemap(g);
            b = aces_filmic_tonemap(b);

            // 将线性值转换为 sRGB gamma
            r = linear_to_srgb_gamma(r);
            g = linear_to_srgb_gamma(g);
            b = linear_to_srgb_gamma(b);

            // 钳制到 [0, 1] 并转换为 8-bit
            auto to_byte = [](float v) -> uint8_t {
                return static_cast<uint8_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
            };

            // OpenCV 使用 BGRA 顺序
            uint8_t alpha = static_cast<uint8_t>(std::clamp(a, 0.0f, 1.0f) * 255.0f + 0.5f);
            dst[x] = cv::Vec4b(to_byte(b), to_byte(g), to_byte(r), alpha);

            src += 4;
        }
        src_row += row_pitch;
    }

    return result;
}

// 将 R10G10B10A2_UNORM HDR 图像转换为 SDR BGRA 图像
// 这种格式通常用于 HDR10，数据已经经过 PQ (ST.2084) 编码
// sdr_white_level：SDR 白点亮度（nits），默认 200
inline cv::Mat hdr_r10g10b10a2_to_sdr_bgra(const void* data, int width, int height, int row_pitch, float sdr_white_level = 200.0f)
{
    cv::Mat result(height, width, CV_8UC4);

    const uint8_t* src_row = static_cast<const uint8_t*>(data);

    // PQ (ST.2084) EOTF 参数
    constexpr float m1 = 0.1593017578125f;
    constexpr float m2 = 78.84375f;
    constexpr float c1 = 0.8359375f;
    constexpr float c2 = 18.8515625f;
    constexpr float c3 = 18.6875f;

    // PQ EOTF: 将 PQ 编码值转换为线性光（输出范围 0-1 对应 0-10000 nits）
    auto pq_eotf = [=](float e) -> float {
        if (e <= 0.0f) return 0.0f;
        float ep = std::pow(e, 1.0f / m2);
        float num = std::max(ep - c1, 0.0f);
        float den = c2 - c3 * ep;
        if (den <= 0.0f) return 0.0f;
        return std::pow(num / den, 1.0f / m1);
    };

    // PQ 峰值 10000 nits，需要归一化到 SDR 白点
    constexpr float pq_peak_nits = 10000.0f;
    const float normalization_factor = 1.0f / (sdr_white_level / pq_peak_nits);

    for (int y = 0; y < height; ++y) {
        const uint32_t* src = reinterpret_cast<const uint32_t*>(src_row);
        cv::Vec4b* dst = result.ptr<cv::Vec4b>(y);

        for (int x = 0; x < width; ++x) {
            uint32_t pixel = src[x];

            // R10G10B10A2_UNORM: R(10bit), G(10bit), B(10bit), A(2bit)
            float r = static_cast<float>(pixel & 0x3FF) / 1023.0f;
            float g = static_cast<float>((pixel >> 10) & 0x3FF) / 1023.0f;
            float b = static_cast<float>((pixel >> 20) & 0x3FF) / 1023.0f;
            float a = static_cast<float>((pixel >> 30) & 0x3) / 3.0f;

            // 应用 PQ EOTF 获得线性光值（0-1 对应 0-10000 nits）
            r = pq_eotf(r);
            g = pq_eotf(g);
            b = pq_eotf(b);

            // 将 nits 归一化到 SDR 白点（sdr_white_level nits = 1.0）
            r *= normalization_factor;
            g *= normalization_factor;
            b *= normalization_factor;

            // 应用 ACES Filmic tone mapping
            r = aces_filmic_tonemap(r);
            g = aces_filmic_tonemap(g);
            b = aces_filmic_tonemap(b);

            // 将线性值转换为 sRGB gamma
            r = linear_to_srgb_gamma(r);
            g = linear_to_srgb_gamma(g);
            b = linear_to_srgb_gamma(b);

            // 钳制到 [0, 1] 并转换为 8-bit
            auto to_byte = [](float v) -> uint8_t {
                return static_cast<uint8_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
            };

            uint8_t alpha = static_cast<uint8_t>(a * 255.0f + 0.5f);
            dst[x] = cv::Vec4b(to_byte(b), to_byte(g), to_byte(r), alpha);
        }
        src_row += row_pitch;
    }

    return result;
}

inline cv::Mat bgra_to_bgr(const cv::Mat& src)
{
    if (src.empty()) {
        return {};
    }

    cv::Mat dst;
    cv::cvtColor(src, dst, cv::COLOR_BGRA2BGR);
    return dst;
}

MAA_CTRL_UNIT_NS_END
