#ifdef __linux__

#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_NS_BEGIN

std::set<ProcessInfo> list_processes()
{
    std::set<ProcessInfo> result;
    DIR* dir = opendir("/proc");
    dirent* de = nullptr;

    while (true) {
        de = readdir(dir);
        if (!de) {
            closedir(dir);
            break;
        }
        if (de->d_type != DT_DIR) {
            continue;
        }

        char* ptr = nullptr;
        char* end = de->d_name + strlen(de->d_name);
        pid_t pid = strtol(de->d_name, &ptr, 10);
        if (ptr != end) {
            continue;
        }
        char buf[32] = { 0 }, path[256] = { 0 };
        sprintf(buf, "/proc/%d/exe", pid);
        auto size = readlink(buf, path, 255);
        if (size != -1) {
            path[size] = 0;
            auto* p = strrchr(path, '/');
            if (p) {
                result.emplace(ProcessInfo { pid, p + 1 });
            }
        }
        else {
            continue;
        }
    }

#ifdef MAA_DEBUG
    LogInfo << "Process list:" << result;
#endif

    return result;
}

std::optional<std::filesystem::path> get_process_path(os_pid pid)
{
    char buf[32] = { 0 }, path[256] = { 0 };
    sprintf(buf, "/proc/%d/exe", pid);
    auto size = readlink(buf, path, 255);
    if (size == -1) {
        auto error = strerror(errno);
        LogError << "Failed to get process path" << VAR(pid) << VAR(error);
        return std::nullopt;
    }

    path[size] = 0;
    return path;
}

MAA_NS_END

#endif
