#ifdef _WIN32

#include "Win32WindowFinder.h"

#include "Utils/Codec.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_TOOLKIT_NS_BEGIN

std::ostream& operator<<(std::ostream& os, const Win32WindowFinder::Window& w)
{
    os << VAR_VOIDP_RAW(w.hwnd) << VAR_RAW(w.class_name) << VAR_RAW(w.window_name);
    return os;
}

size_t Win32WindowFinder::find_window(std::string_view class_name, std::string_view window_name)
{
    auto windows = list_windows();

    windows_.clear();
    for (const auto& w : windows) {
        bool same_class = class_name.empty() || w.class_name == class_name;
        bool same_window = window_name.empty() || w.window_name == window_name;
        if (same_class && same_window) {
            windows_.emplace_back(w.hwnd);
        }
    }
    return windows_.size();
}

size_t Win32WindowFinder::search_window(std::string_view class_name, std::string_view window_name)
{
    auto windows = list_windows();

    windows_.clear();
    for (const auto& w : windows) {
        bool same_class = class_name.empty() || w.class_name.find(class_name) != std::string::npos;
        bool same_window =
            window_name.empty() || w.window_name.find(window_name) != std::string::npos;
        if (same_class && same_window) {
            windows_.emplace_back(w.hwnd);
        }
    }
    return windows_.size();
}

MaaWin32Hwnd Win32WindowFinder::get_cursor_window() const
{
    POINT pt {};
    if (!GetCursorPos(&pt)) {
        return nullptr;
    }

    HWND hwnd = WindowFromPoint(pt);
    if (hwnd == NULL) {
        return nullptr;
    }

    return reinterpret_cast<MaaWin32Hwnd>(hwnd);
}

MaaWin32Hwnd Win32WindowFinder::get_desktop_window() const
{
    return GetDesktopWindow();
}

MaaWin32Hwnd Win32WindowFinder::get_foreground_window() const
{
    return GetForegroundWindow();
}

std::vector<Win32WindowFinder::Window> Win32WindowFinder::list_windows() const
{
    std::vector<Window> windows;

    for (HWND hwnd = GetTopWindow(NULL); hwnd != NULL; hwnd = GetNextWindow(hwnd, GW_HWNDNEXT)) {
        if (!IsWindowVisible(hwnd)) {
            continue;
        }

        std::wstring class_name(256, '\0');
        GetClassNameW(hwnd, class_name.data(), static_cast<int>(class_name.size()));

        std::wstring window_name(256, '\0');
        GetWindowTextW(hwnd, window_name.data(), static_cast<int>(window_name.size()));

        windows.emplace_back(Window { .hwnd = reinterpret_cast<MaaWin32Hwnd>(hwnd),
                                      .class_name = from_u16(class_name),
                                      .window_name = from_u16(window_name) });
    }

#ifdef MAA_DEBUG
    LogInfo << "Window list:" << windows;
#endif

    return windows;
}

MAA_TOOLKIT_NS_END

#endif