#ifdef _WIN32

#include "DesktopWindowWin32Finder.h"

#include "Common/Win32WindowFinder.hpp"
#include "MaaUtils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

std::vector<DesktopWindow> DesktopWindowWin32Finder::find_all() const
{
    LogFunc;

    std::vector<DesktopWindow> windows;

    for (HWND hwnd : visible_top_level_windows()) {
        if (auto names = win32_window_names(hwnd)) {
            windows.emplace_back(DesktopWindow { hwnd, std::move(names->class_name), std::move(names->window_name) });
        }
    }

#ifdef MAA_DEBUG
    LogInfo << "Window list:" << windows;
#endif

    return windows;
}

MAA_TOOLKIT_NS_END

#endif // _WIN32
