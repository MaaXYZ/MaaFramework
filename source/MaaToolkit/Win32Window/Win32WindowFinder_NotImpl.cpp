#ifndef _WIN32

#include "Win32WindowFinder.h"

#include "Utils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

std::ostream& operator<<(std::ostream& os, const Win32WindowFinder::Window& w)
{
    os << VAR_VOIDP_RAW(w.hwnd) << VAR_RAW(w.class_name) << VAR_RAW(w.window_name);
    return os;
}

size_t Win32WindowFinder::find_window(std::string_view class_name, std::string_view window_name)
{
    LogError << "Not implemented" << VAR(class_name) << VAR(window_name);
    return 0;
}

size_t Win32WindowFinder::search_window(std::string_view class_name, std::string_view window_name)
{
    LogError << "Not implemented" << VAR(class_name) << VAR(window_name);
    return 0;
}

MaaWin32Hwnd Win32WindowFinder::get_cursor_window() const
{
    LogError << "Not implemented";
    return nullptr;
}

MaaWin32Hwnd Win32WindowFinder::get_desktop_window() const
{
    LogError << "Not implemented";
    return nullptr;
}

MaaWin32Hwnd Win32WindowFinder::get_foreground_window() const
{
    LogError << "Not implemented";
    return nullptr;
}

std::vector<Win32WindowFinder::Window> Win32WindowFinder::list_windows() const
{
    LogError << "Not implemented";
    return {};
}

MAA_TOOLKIT_NS_END

#endif