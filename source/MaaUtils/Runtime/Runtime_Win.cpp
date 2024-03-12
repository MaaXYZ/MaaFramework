#ifdef _WIN32
#include "Utils/Runtime.h"

#include "Utils/Platform.h"
#include "Utils/SafeWindows.hpp"

MAA_NS_BEGIN

static std::filesystem::path s_library_dir_cache;

const std::filesystem::path& library_dir()
{
    return s_library_dir_cache;
}

void init_library_dir(HINSTANCE hinstDLL)
{
    char buffer[MAX_PATH + 1] = { 0 };
    GetModuleFileName(hinstDLL, buffer, MAX_PATH);
    s_library_dir_cache = MAA_NS::path(buffer).parent_path();
}

MAA_NS_END

// https://learn.microsoft.com/zh-cn/windows/win32/dlls/dllmain
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL, // handle to DLL module
    DWORD fdwReason,    // reason for calling function
    LPVOID lpvReserved) // reserved
{
    // Perform actions based on the reason for calling.
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        // Initialize once for each new process.
        // Return FALSE to fail DLL load.
        MAA_NS::init_library_dir(hinstDLL);
        break;

    case DLL_THREAD_ATTACH:
        // Do thread-specific initialization.
        break;

    case DLL_THREAD_DETACH:
        // Do thread-specific cleanup.
        break;

    case DLL_PROCESS_DETACH:

        if (lpvReserved != nullptr) {
            break; // do not do cleanup if process termination scenario
        }

        // Perform any necessary cleanup.
        break;
    }
    return TRUE; // Successful DLL_PROCESS_ATTACH.
}

#endif