#ifndef _WIN32

#include "Win32WindowFinder.h"

#include "Utils/Logger.h"

std::ostream& operator<<(std::ostream& os, const MaaWin32WindowAPI::Window& w)
{
    os << VAR_VOIDP_RAW(w.hwnd) << VAR_RAW(w.class_name) << VAR_RAW(w.window_name);
    return os;
}

MAA_TOOLKIT_NS_BEGIN

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

size_t Win32WindowFinder::list_windows()
{
    LogError << "Not implemented";
    return 0;
}

void* Win32WindowFinder::get_cursor_window() const
{
    LogError << "Not implemented";
    return nullptr;
}

void* Win32WindowFinder::get_desktop_window() const
{
    LogError << "Not implemented";
    return nullptr;
}

void* Win32WindowFinder::get_foreground_window() const
{
    LogError << "Not implemented";
    return nullptr;
}

std::vector<Win32WindowFinder::Window> Win32WindowFinder::_list_windows() const
{
    LogError << "Not implemented";
    return {};
}

std::optional<std::string> Win32WindowFinder::get_class_name(void* hwnd) const
{
    LogError << "Not implemented" << VAR_VOIDP(hwnd);
    return std::nullopt;
}

std::optional<std::string> Win32WindowFinder::get_window_name(void* hwnd) const
{
    LogError << "Not implemented" << VAR_VOIDP(hwnd);
    return std::nullopt;
}

MAA_TOOLKIT_NS_END

#endif
