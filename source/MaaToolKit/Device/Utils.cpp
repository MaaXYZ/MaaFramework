#include "Utils.h"

#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"
#include "Utils/Ranges.hpp"

#if defined(__APPLE__)

#include <libproc.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>

MAA_TOOLKIT_DEVICE_NS_BEGIN

std::vector<ProcessInfo> get_processes_info()
{
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    size_t size;
    sysctl(mib, 4, NULL, &size, NULL, 0);

    int proc_count = size / sizeof(struct kinfo_proc);
    kinfo_proc* proc_list = new kinfo_proc[proc_count];

    sysctl(mib, 4, proc_list, &size, NULL, 0);

    std::vector<ProcessInfo> result;
    for (int i = 0; i < proc_count; i++) {
        result.push_back({ proc_list[i].kp_proc.p_pid, proc_list[i].kp_proc.p_comm });
    }

    delete[] proc_list;

    return result;
}

os_string get_process_path(os_pid pid)
{
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
    proc_pidpath(pid, pathbuf, sizeof(pathbuf));
    return pathbuf;
}

MAA_TOOLKIT_DEVICE_NS_END

#endif
