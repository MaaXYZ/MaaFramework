#pragma once

#include <utility>

#include "Common/Conf.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "MaaUtils/SafeWindows.hpp"

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

// Ensure the window's client area is fully visible on the monitor.
// If the window extends beyond the monitor bounds, move it back.
// If the client area is larger than the monitor, resize the window.
//
// Note: the reference rect is the full monitor (rcMonitor), NOT the work
// area (rcWork). Windows are allowed to overlap the taskbar - typically a
// borderless fullscreen game window. Anchoring to rcWork would shrink or
// move such windows above the taskbar, deforming the game view.
inline bool ensure_window_on_screen(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }

    // Don't adjust maximized or minimized windows
    if (IsZoomed(hwnd) || IsIconic(hwnd)) {
        return true;
    }

    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (!monitor) {
        return false;
    }

    MONITORINFO mi = { };
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfo(monitor, &mi)) {
        return false;
    }

    RECT monitor_rect = mi.rcMonitor;
    int monitor_w = monitor_rect.right - monitor_rect.left;
    int monitor_h = monitor_rect.bottom - monitor_rect.top;

    // Get window rect and client rect to calculate frame sizes
    RECT window_rect = { };
    if (!GetWindowRect(hwnd, &window_rect)) {
        return false;
    }

    // A window that already covers the whole monitor (typically a borderless
    // fullscreen game window) is expected to overlap the taskbar. Moving or
    // resizing it would deform the game view and keep the taskbar visible,
    // so leave it untouched. (Slack tolerates DWM invisible borders.)
    const LONG kMonitorSlack = 8;
    bool covers_monitor = window_rect.left <= mi.rcMonitor.left + kMonitorSlack
                          && window_rect.top <= mi.rcMonitor.top + kMonitorSlack
                          && window_rect.right >= mi.rcMonitor.right - kMonitorSlack
                          && window_rect.bottom >= mi.rcMonitor.bottom - kMonitorSlack;
    if (is_fullscreen(hwnd) && covers_monitor) {
        LogDebug << "Skip ensure_window_on_screen: fullscreen-coverage window";
        return true;
    }

    RECT client_rect = { };
    if (!GetClientRect(hwnd, &client_rect)) {
        return false;
    }

    POINT client_origin = { 0, 0 };
    if (!ClientToScreen(hwnd, &client_origin)) {
        return false;
    }

    int frame_left = client_origin.x - window_rect.left;
    int frame_top = client_origin.y - window_rect.top;
    int frame_right = window_rect.right - (client_origin.x + client_rect.right);
    int frame_bottom = window_rect.bottom - (client_origin.y + client_rect.bottom);

    int client_w = client_rect.right - client_rect.left;
    int client_h = client_rect.bottom - client_rect.top;

    bool need_change = false;

    // If client area is larger than monitor, cap to monitor size
    int new_client_w = client_w;
    int new_client_h = client_h;

    if (new_client_w > monitor_w) {
        new_client_w = monitor_w;
        need_change = true;
    }
    if (new_client_h > monitor_h) {
        new_client_h = monitor_h;
        need_change = true;
    }

    // Calculate desired client area position
    int new_client_x = client_origin.x;
    int new_client_y = client_origin.y;

    // Adjust right/bottom first
    if (new_client_x + new_client_w > monitor_rect.right) {
        new_client_x = monitor_rect.right - new_client_w;
        need_change = true;
    }
    if (new_client_y + new_client_h > monitor_rect.bottom) {
        new_client_y = monitor_rect.bottom - new_client_h;
        need_change = true;
    }

    // Then adjust left/top (takes priority)
    if (new_client_x < monitor_rect.left) {
        new_client_x = monitor_rect.left;
        need_change = true;
    }
    if (new_client_y < monitor_rect.top) {
        new_client_y = monitor_rect.top;
        need_change = true;
    }

    if (!need_change) {
        return true;
    }

    // Convert client coordinates back to window coordinates
    int new_window_x = new_client_x - frame_left;
    int new_window_y = new_client_y - frame_top;
    int new_window_w = new_client_w + frame_left + frame_right;
    int new_window_h = new_client_h + frame_top + frame_bottom;

    LogInfo << "Moving/resizing window to keep client area on screen" << VAR(new_window_x) << VAR(new_window_y) << VAR(new_window_w)
            << VAR(new_window_h);

    // SWP_ASYNCWINDOWPOS: avoid blocking if the target window's thread is busy/hung
    return SetWindowPos(
               hwnd,
               nullptr,
               new_window_x,
               new_window_y,
               new_window_w,
               new_window_h,
               SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS)
           != 0;
}

inline cv::Mat bgra_to_bgr(const cv::Mat& src)
{
    if (src.empty()) {
        return { };
    }

    cv::Mat dst;
    cv::cvtColor(src, dst, cv::COLOR_BGRA2BGR);
    return dst;
}

MAA_CTRL_UNIT_NS_END
