#include "MaaToolkit/Win32/MaaToolkitWin32Window.h"

#include <string_view>

#include "Buffer/StringBuffer.hpp"
#include "Utils/Logger.h"
#include "Win32Window/Win32WindowFinder.h"

auto& win32_mgr = MAA_TOOLKIT_NS::Win32WindowFinder::get_instance();

MaaSize MaaToolkitFindWindow(const char* class_name, const char* window_name)
{
    LogInfo << VAR(class_name) << VAR(window_name);

    return win32_mgr.find_window(class_name, window_name);
}

MaaSize MaaToolkitSearchWindow(const char* class_name, const char* window_name)
{
    LogInfo << VAR(class_name) << VAR(window_name);

    return win32_mgr.search_window(class_name, window_name);
}

MaaSize MaaToolkitListWindows()
{
    LogInfo;

    return win32_mgr.list_windows();
}

void* MaaToolkitGetWindow(MaaSize index)
{
    return win32_mgr.found_windows().at(index).hwnd;
}

void* MaaToolkitGetCursorWindow()
{
    LogInfo;

    return win32_mgr.get_cursor_window();
}

void* MaaToolkitGetDesktopWindow()
{
    LogInfo;

    return win32_mgr.get_desktop_window();
}

void* MaaToolkitGetForegroundWindow()
{
    LogInfo;

    return win32_mgr.get_foreground_window();
}

MaaBool MaaToolkitGetWindowClassName(void* hwnd, MaaStringBuffer* buffer)
{
    auto opt = win32_mgr.get_class_name(hwnd);
    if (!opt) {
        return false;
    }

    buffer->set(std::move(*opt));
    return true;
}

MaaBool MaaToolkitGetWindowWindowName(void* hwnd, MaaStringBuffer* buffer)
{
    auto opt = win32_mgr.get_window_name(hwnd);
    if (!opt) {
        return false;
    }

    buffer->set(std::move(*opt));
    return true;
}
