#pragma once

#include <utility>

#include "Conf/Conf.h"
#include "Utils/NoWarningCV.hpp"
#include "Utils/SafeWindows.hpp"

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
