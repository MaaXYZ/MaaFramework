#include "library.h"

#if defined(_WIN32)

#include <Windows.h>
#include <psapi.h>

std::filesystem::path get_library_path(void* addr)
{
    WCHAR path_buf[MAX_PATH + 5];
    DWORD path_len = GetMappedFileNameW(GetCurrentProcess(), addr, path_buf, MAX_PATH);

    if (path_len == 0) {
        return {};
    }

    return { path_buf };
}

#else

#include <dlfcn.h>

std::filesystem::path get_library_path(void* addr)
{
    Dl_info dl_info {};
    if (dladdr(addr, &dl_info) == 0) {
        return {};
    }

    return { dl_info.dli_fname };
}

#endif
