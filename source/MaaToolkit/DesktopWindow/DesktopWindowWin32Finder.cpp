#ifdef _WIN32

#include "DesktopWindowWin32Finder.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_TOOLKIT_NS_BEGIN

std::vector<DesktopWindow> DesktopWindowWin32Finder::find_all() const
{
    LogFunc;

    std::vector<DesktopWindow> windows;

    for (HWND hwnd = GetTopWindow(NULL); hwnd != NULL; hwnd = GetNextWindow(hwnd, GW_HWNDNEXT)) {
        if (!IsWindowVisible(hwnd)) {
            continue;
        }

        std::wstring class_name(256, '\0');
        GetClassNameW(hwnd, class_name.data(), static_cast<int>(class_name.size()));

        std::wstring window_name(256, '\0');
        GetWindowTextW(hwnd, window_name.data(), static_cast<int>(window_name.size()));

        windows.emplace_back(
            DesktopWindow {
                .hwnd = hwnd,
                .class_name = class_name,
                .window_name = window_name,
            });
    }

#ifdef MAA_DEBUG
    LogInfo << "Window list:" << windows;
#endif

    return windows;
}

MAA_TOOLKIT_NS_END

#endif // _WIN32
