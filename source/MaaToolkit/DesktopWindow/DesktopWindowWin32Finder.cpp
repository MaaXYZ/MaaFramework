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

        // Full image path of the owning process, e.g. "C:\...\GeForceNOW.exe".
        // Used by cloud/GFN controller matching to disambiguate a generic window
        // class (CEFCLIENT) by process. Empty if the process cannot be queried.
        std::wstring process_path;
        DWORD process_id = 0;
        GetWindowThreadProcessId(hwnd, &process_id);
        if (process_id != 0) {
            HANDLE process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
            if (process_handle) {
                std::wstring buffer(1024, L'\0');
                DWORD buffer_size = static_cast<DWORD>(buffer.size());
                if (QueryFullProcessImageNameW(process_handle, 0, buffer.data(), &buffer_size)) {
                    buffer.resize(buffer_size);
                    process_path = std::move(buffer);
                }
                CloseHandle(process_handle);
            }
        }

        windows.emplace_back(
            DesktopWindow {
                .hwnd = hwnd,
                .class_name = class_name,
                .window_name = window_name,
                .process_path = process_path,
            });
    }

#ifdef MAA_DEBUG
    LogInfo << "Window list:" << windows;
#endif

    return windows;
}

MAA_TOOLKIT_NS_END

#endif // _WIN32
