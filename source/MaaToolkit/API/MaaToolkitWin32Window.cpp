#include "MaaToolkit/Win32/MaaToolkitWin32Window.h"

#include <string_view>

#include "Buffer/StringBuffer.hpp"
#include "Utils/Logger.h"
#include "Win32Window/Win32WindowFinder.h"

auto& win32_mgr = MAA_TOOLKIT_NS::Win32WindowFinder::get_instance();

MaaSize MaaToolkitFindWindow(MaaStringView class_name, MaaStringView window_name)
{
    LogInfo << VAR(class_name) << VAR(window_name);

    return win32_mgr.find_window(class_name, window_name);
}

MaaSize MaaToolkitSearchWindow(MaaStringView class_name, MaaStringView window_name)
{
    LogInfo << VAR(class_name) << VAR(window_name);

    return win32_mgr.search_window(class_name, window_name);
}

MaaWin32Hwnd MaaToolkitGetWindow(MaaSize index)
{
    return win32_mgr.found_windows().at(index).hwnd;
}

MaaWin32Hwnd MaaToolkitGetCursorWindow()
{
    LogInfo;

    return win32_mgr.get_cursor_window();
}

MaaWin32Hwnd MaaToolkitGetDesktopWindow()
{
    LogInfo;

    return win32_mgr.get_desktop_window();
}

MaaWin32Hwnd MaaToolkitGetForegroundWindow()
{
    LogInfo;

    return win32_mgr.get_foreground_window();
}

MaaBool MaaToolkitGetWindowClassName(MaaWin32Hwnd hwnd, MaaStringBufferHandle buffer)
{
    auto opt = win32_mgr.get_class_name(hwnd);
    if (!opt) {
        return false;
    }

    buffer->set(std::move(*opt));
    return true;
}

MaaBool MaaToolkitGetWindowWindowName(MaaWin32Hwnd hwnd, MaaStringBufferHandle buffer)
{
    auto opt = win32_mgr.get_window_name(hwnd);
    if (!opt) {
        return false;
    }

    buffer->set(std::move(*opt));
    return true;
}
