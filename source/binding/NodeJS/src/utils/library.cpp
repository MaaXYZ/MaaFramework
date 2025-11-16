#include "library.h"

#if defined(_WIN32)

#include <Windows.h>
#include <fileapi.h>
#include <psapi.h>

// \Device\HarddiskVolume3\xxx -> C:\xxx
static std::wstring NtPathToDosPath(std::wstring nt_path)
{
    WCHAR drive_buffers[512];

    DWORD len = GetLogicalDriveStringsW(512, drive_buffers);
    if (!len) {
        return L"";
    }

    std::vector<std::wstring> drives;
    WCHAR* p = drive_buffers;
    while (*p) {
        drives.emplace_back(p);
        p += wcslen(p) + 1;
    }

    WCHAR device_paths[512];
    for (const auto& drive : drives) {
        std::wstring drive_name = drive.substr(0, 2);

        DWORD ret = QueryDosDeviceW(drive_name.c_str(), device_paths, 512);
        if (!ret) {
            continue;
        }

        std::wstring dev { device_paths };
        if (nt_path.rfind(dev, 0) == 0) {
            nt_path.replace(0, dev.size(), drive_name);
            return nt_path;
        }
    }

    return L"";
}

std::filesystem::path get_library_path(void* addr)
{
    WCHAR path_buf[MAX_PATH + 5];
    DWORD path_len = GetMappedFileNameW(GetCurrentProcess(), addr, path_buf, MAX_PATH);

    if (path_len == 0) {
        return {};
    }

    return { NtPathToDosPath(path_buf) };
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
