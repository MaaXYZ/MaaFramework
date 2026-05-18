#pragma once

#include <atomic>
#include <utility>

#include "Common/Conf.h"
#include "Input/InputUtils.h"
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

// 把目标窗口拉到 Z 序顶部并尝试激活，专供不支持后台的截图方式
// (GDI / ScreenDC / DXGI_DesktopDup / DXGI_DesktopDup_Window) 在每次 screencap() 前调用。
//
// 行为：
//   - fast path: 窗口已是前台时仅做一次 GetForegroundWindow 检查 (~0.1us)，命中即返回。
//   - 节流: 一次实际拉前台后 kForegroundPullCooldownMs 内不再抢前台，给用户切到客户端
//     UI 操作（如停止任务）的窗口期；冷却结束后若窗口仍未在前台则再拉一次，从而在
//     长期挂机场景下也能周期性自救。
//   - 拉前台直接复用输入侧的 ensure_foreground_and_topmost：SetForegroundWindow 被
//     Windows 前台锁定拒绝时，输入侧已有"再 SetWindowPos(HWND_TOP)"作为兜底。对截图
//     场景而言，只要 Z 序提升 DWM 就会重新合成正确画面，焦点是否归位无关紧要。
//   - 与输入侧调用 ensure_foreground_and_topmost 的时机平行：输入侧只在 action 内触发，
//     无法挽救"识别一直失败、永远走不到 action"的死锁场景，也覆盖不到 connect 时的
//     speed_test；这里在截图开头补上，两条路径通过 Windows 的真实前台状态自然仲裁。
inline void ensure_window_foreground_for_screencap(HWND hwnd)
{
    // 节流冷却时长，给用户操作客户端 UI（如停止任务）的窗口期。
    constexpr DWORD kForegroundPullCooldownMs = 5000;

    // 最小化窗口由 FramePool/PrintWindow 各自的 PseudoMinimizeHelper 处理；不支持后台
    // 的截图方式对最小化窗口本来就截不出有效内容，这里跳过避免对用户状态的额外干扰。
    // ensure_foreground_and_topmost 内部已有 hwnd 空检查和 fast path，此处不重复。
    if (!hwnd || IsIconic(hwnd) || GetForegroundWindow() == hwnd) {
        return;
    }

    // 节流：使用 GetTickCount（与 ensure_foreground_and_topmost 内部 sleep 计时体系一致；
    // 49.7 天回绕，但无符号差值在回绕后仍正确）。多 controller 共存时共享同一冷却计时器。
    static std::atomic<DWORD> s_last_pull_tick { 0 };
    DWORD now = GetTickCount();
    DWORD last = s_last_pull_tick.load(std::memory_order_relaxed);
    if (last != 0 && (now - last) < kForegroundPullCooldownMs) {
        return;
    }

    LogInfo << "Pulling window to foreground for screencap" << VAR_VOIDP(hwnd);
    ensure_foreground_and_topmost(hwnd);
    s_last_pull_tick.store(now, std::memory_order_relaxed);
}

// Ensure the window's client area is fully visible on the monitor.
// If the window extends beyond the monitor bounds, move it back.
// If the client area is larger than the monitor, resize the window.
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

    RECT monitor_rect = mi.rcWork;
    int monitor_w = monitor_rect.right - monitor_rect.left;
    int monitor_h = monitor_rect.bottom - monitor_rect.top;

    // Get window rect and client rect to calculate frame sizes
    RECT window_rect = { };
    if (!GetWindowRect(hwnd, &window_rect)) {
        return false;
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
