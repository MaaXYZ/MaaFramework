#include "MaaToolKit/Win32/MaaToolKitWin32Window.h"

#include <string_view>

#include "Utils/Logger.h"
#include "Win32Window/Win32WindowFinder.h"

auto& win32_mgr = MAA_TOOLKIT_WIN32_NS::Win32WindowFinder::get_instance();

MaaSize MaaToolKitFindWindow(MaaStringView class_name, MaaStringView window_name)
{
    LogInfo << VAR(class_name) << VAR(window_name);

    return win32_mgr.find_window(class_name, window_name);
}

MaaSize MaaToolKitSearchWindow(MaaStringView class_name, MaaStringView window_name)
{
    LogInfo << VAR(class_name) << VAR(window_name);

    return win32_mgr.search_window(class_name, window_name);
}

MaaWin32Hwnd MaaToolKitGetWindow(MaaSize index)
{
    return win32_mgr.found_window().at(index);
}

MaaWin32Hwnd MaaToolKitGetCursorWindow()
{
    LogInfo;

    return win32_mgr.get_cursor_window();
}
