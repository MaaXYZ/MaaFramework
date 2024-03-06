#include "MaaToolkit/Win32/MaaToolkitWin32Window.h"

#include <string_view>

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
    return win32_mgr.found_window().at(index);
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