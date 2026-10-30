#include "MaaUtils/SafeWindows.hpp"

#include <dwmapi.h>

#include <array>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

namespace
{
struct Placement
{
    int x, y, width, height;
    UINT flags;
    HWND after;
};

struct Script
{
    RECT monitor { 0, 0, 1920, 1080 };
    RECT work { 0, 0, 1920, 1040 };
    RECT outer { 0, 0, 1920, 1080 };
    RECT client { 0, 0, 1920, 1080 };
    POINT origin { 0, 0 };
    LONG_PTR style = WS_POPUP;
    bool valid = true;
    bool zoomed = false;
    bool iconic = false;
    // 1: monitor, 2: info, 3: outer, 4: client, 5: origin, 6: position.
    int failure = 0;
    int queries = 0;
    int positions = 0;
    std::optional<Placement> placement;
};

Script* active = nullptr;
int cases = 0;

void check(bool condition, std::string_view scenario)
{
    if (!condition) {
        std::cerr << "FAILED: " << scenario << '\n';
        std::exit(1);
    }
}

BOOL valid(HWND window)
{
    return active ? active->valid : ::IsWindow(window);
}

BOOL zoomed(HWND window)
{
    return active ? active->zoomed : ::IsZoomed(window);
}

BOOL iconic(HWND window)
{
    return active ? active->iconic : ::IsIconic(window);
}

HMONITOR script_monitor(HWND window, DWORD flags)
{
    if (!active) {
        return ::MonitorFromWindow(window, flags);
    }
    check(flags == MONITOR_DEFAULTTONEAREST, "nearest monitor selection");
    ++active->queries;
    return active->failure == 1 ? nullptr : reinterpret_cast<HMONITOR>(1);
}

BOOL info(HMONITOR handle, MONITORINFO* output)
{
    if (!active) {
        return ::GetMonitorInfoW(handle, output);
    }
    check(output->cbSize == sizeof(MONITORINFO), "monitor structure prefix");
    output->rcMonitor = active->monitor;
    output->rcWork = active->work;
    return active->failure != 2;
}

BOOL outer(HWND window, RECT* output)
{
    if (!active) {
        return ::GetWindowRect(window, output);
    }
    *output = active->outer;
    return active->failure != 3;
}

BOOL client(HWND window, RECT* output)
{
    if (!active) {
        return ::GetClientRect(window, output);
    }
    *output = active->client;
    return active->failure != 4;
}

BOOL origin(HWND window, POINT* output)
{
    if (!active) {
        return ::ClientToScreen(window, output);
    }
    check(output->x == 0 && output->y == 0, "client origin query");
    *output = active->origin;
    return active->failure != 5;
}

LONG_PTR style(HWND window, int index)
{
    if (!active) {
        return ::GetWindowLongPtrW(window, index);
    }
    check(index == GWL_STYLE, "window style index");
    return active->style;
}

BOOL position(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
{
    if (!active) {
        return ::SetWindowPos(window, after, x, y, width, height, flags);
    }
    ++active->positions;
    active->placement = Placement { x, y, width, height, flags, after };
    return active->failure != 6;
}
}

// Intercept native queries only at this translation unit's header boundary.
#define IsWindow valid
#define IsZoomed zoomed
#define IsIconic iconic
#define MonitorFromWindow script_monitor
#undef GetMonitorInfo
#define GetMonitorInfo info
#define GetWindowRect outer
#define GetClientRect client
#define ClientToScreen origin
#undef GetWindowLongPtr
#define GetWindowLongPtr style
#define SetWindowPos position
#include "Screencap/HwndUtils.hpp"
#undef IsWindow
#undef IsZoomed
#undef IsIconic
#undef MonitorFromWindow
#undef GetMonitorInfo
#undef GetWindowRect
#undef GetClientRect
#undef ClientToScreen
#undef GetWindowLongPtr
#undef SetWindowPos

namespace
{
bool run(Script& script, HWND window = reinterpret_cast<HWND>(1))
{
    ++cases;
    active = &script;
    const bool result = MaaNS::CtrlUnitNs::ensure_window_on_screen(window);
    active = nullptr;
    if (script.placement) {
        check(script.placement->after == nullptr, "no z-order target");
        check(script.placement->flags == (SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS), "original position flags");
    }
    return result;
}

void scripted_cases()
{
    Script exact;
    check(run(exact) && !exact.placement, "exact fullscreen client is untouched");
    Script border;
    border.outer = { -8, -8, 1928, 1088 };
    check(run(border) && !border.placement, "invisible outer border does not change client equality");
    Script ordinary;
    ordinary.style = WS_OVERLAPPEDWINDOW;
    check(run(ordinary) && ordinary.placement && ordinary.placement->height == 1040, "ordinary fullscreen-sized client uses work area");
    Script taskbar;
    taskbar.style = WS_OVERLAPPEDWINDOW;
    taskbar.outer = { 20, 980, 420, 1080 };
    taskbar.origin = { 20, 980 };
    taskbar.client = { 0, 0, 400, 100 };
    check(run(taskbar) && taskbar.placement && taskbar.placement->y == 940, "ordinary client below work area is moved above taskbar");

    for (int edge = 0; edge < 4; ++edge) {
        for (const LONG delta : { -9L, -8L, -7L, -1L, 1L, 7L, 8L, 9L }) {
            Script shifted;
            if (edge == 0) {
                shifted.outer.left += delta;
            }
            if (edge == 1) {
                shifted.outer.top += delta;
            }
            if (edge == 2) {
                shifted.outer.right += delta;
            }
            if (edge == 3) {
                shifted.outer.bottom += delta;
            }
            shifted.origin = { shifted.outer.left, shifted.outer.top };
            shifted.client = { 0, 0, shifted.outer.right - shifted.outer.left, shifted.outer.bottom - shifted.outer.top };
            check(run(shifted) && shifted.positions == 1, "every client-edge offset must reach work-area correction");
            const auto& p = *shifted.placement;
            check(
                p.x >= shifted.work.left && p.y >= shifted.work.top && p.x + p.width <= shifted.work.right
                    && p.y + p.height <= shifted.work.bottom,
                "corrected borderless client fits work area");
        }
    }

    Script negative;
    negative.monitor = { -1920, -1080, 0, 0 };
    negative.work = { -1880, -1080, 0, 0 };
    negative.outer = negative.monitor;
    negative.origin = { -1920, -1080 };
    check(run(negative) && !negative.placement, "negative monitor coordinates preserve exact fullscreen");
    for (const RECT work : { RECT { 40, 0, 1920, 1080 },
                             RECT { 0, 40, 1920, 1080 },
                             RECT { 0, 0, 1880, 1080 },
                             RECT { 0, 0, 1920, 1040 },
                             RECT { 0, 0, 1920, 1080 } }) {
        Script fullscreen;
        fullscreen.work = work;
        check(run(fullscreen) && !fullscreen.placement, "taskbar edge or autohide does not alter exact fullscreen");
    }
    Script invalid;
    check(!run(invalid, nullptr) && invalid.queries == 0, "null HWND fails before native monitor queries");
    invalid.valid = false;
    check(!run(invalid) && invalid.queries == 0, "invalid HWND fails before native monitor queries");
    Script maximized;
    maximized.zoomed = true;
    check(run(maximized) && maximized.queries == 0, "maximized exemption is preserved");
    Script minimized;
    minimized.iconic = true;
    check(run(minimized) && minimized.queries == 0, "minimized exemption is preserved");
    for (int failure = 1; failure <= 6; ++failure) {
        Script failed;
        failed.failure = failure;
        failed.style = WS_OVERLAPPEDWINDOW;
        check(!run(failed), "native query/position failure is propagated");
        check(failed.positions == (failure == 6 ? 1 : 0), "failed queries cannot reposition the window");
    }
    std::cout << cases << " actual-header scripted geometry cases passed\n";
}

void desktop_cases(bool verify_pixels)
{
    const auto previous = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    check(previous != nullptr, "set test thread DPI context");
    MONITORINFO mi { };
    mi.cbSize = sizeof(mi);
    check(GetMonitorInfoW(MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &mi), "query primary desktop");
    const auto brush = CreateSolidBrush(RGB(40, 180, 90));
    check(brush != nullptr, "create owned fixture brush");
    WNDCLASSW wc { };
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hbrBackground = brush;
    wc.lpszClassName = L"MaaWindowGeometryRegression";
    check(RegisterClassW(&wc) != 0, "register owned fixture class");
    for (int scenario = 0; scenario < 3; ++scenario) {
        const DWORD window_style = scenario == 2 ? WS_OVERLAPPEDWINDOW : WS_POPUP;
        const RECT initial = mi.rcMonitor;
        HWND window = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_NOACTIVATE,
            wc.lpszClassName,
            L"Maa geometry regression",
            window_style,
            initial.left - (scenario == 1 ? 1 : 0),
            initial.top,
            initial.right - initial.left,
            initial.bottom - initial.top,
            nullptr,
            nullptr,
            wc.hInstance,
            nullptr);
        check(window != nullptr, "create owned fixture window");
        ShowWindow(window, SW_SHOWNOACTIVATE);
        UpdateWindow(window);
        check(MaaNS::CtrlUnitNs::ensure_window_on_screen(window), "actual Win32 correction succeeds");
        RECT actual { };
        POINT point { 0, 0 };
        check(GetClientRect(window, &actual) && ClientToScreen(window, &point), "read corrected native client");
        const RECT screen { point.x, point.y, point.x + actual.right, point.y + actual.bottom };
        if (scenario == 0) {
            check(EqualRect(&screen, &mi.rcMonitor), "actual fullscreen client remains equal to monitor");
        }
        else {
            check(
                screen.left >= mi.rcWork.left && screen.top >= mi.rcWork.top && screen.right <= mi.rcWork.right
                    && screen.bottom <= mi.rcWork.bottom,
                "actual shifted/ordinary client is corrected into work area");
        }
        check(
            screen.left >= mi.rcMonitor.left && screen.top >= mi.rcMonitor.top && screen.right <= mi.rcMonitor.right
                && screen.bottom <= mi.rcMonitor.bottom,
            "actual client permits a nonnegative in-bounds monitor crop");
        if (verify_pixels) {
            UpdateWindow(window);
            check(SUCCEEDED(DwmFlush()), "complete fixture presentation");
            HDC dc = GetDC(nullptr);
            check(dc != nullptr, "obtain screen DC");
            const COLORREF pixel = GetPixel(dc, point.x + actual.right / 2, point.y + actual.bottom / 2);
            check(ReleaseDC(nullptr, dc) != 0, "release screen DC");
            if (pixel != RGB(40, 180, 90)) {
                std::cerr << "scenario=" << scenario << " actual pixel=" << pixel << " expected=" << RGB(40, 180, 90) << '\n';
            }
            check(pixel == RGB(40, 180, 90), "visible owned client pixel matches fixture");
        }
        check(DestroyWindow(window), "destroy owned fixture window");
    }
    // UnregisterClass owns deletion of the registered class background brush.
    check(UnregisterClassW(wc.lpszClassName, wc.hInstance), "unregister fixture class");
    check(SetThreadDpiAwarenessContext(previous) != nullptr, "restore test thread DPI context");
    std::cout << "3 native desktop geometry/crop cases passed; screen pixels " << (verify_pixels ? "verified" : "not checked") << '\n';
}
}

int main(int argc, char** argv)
{
    if (argc == 2 && (std::string_view(argv[1]) == "--desktop" || std::string_view(argv[1]) == "--desktop-geometry")) {
        desktop_cases(std::string_view(argv[1]) == "--desktop");
        return 0;
    }
    if (argc != 1) {
        std::cerr << "usage: window_geometry_test [--desktop|--desktop-geometry]\n";
        return 2;
    }
    scripted_cases();
}
