#ifdef __APPLE__

#include "Utils/Platform.h"

#include <cstdlib>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <libproc.h>
#include <sys/sysctl.h>

MAA_NS_BEGIN

std::set<ProcessInfo> list_processes()
{
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
    size_t size = 0;
    sysctl(mib, 4, NULL, &size, NULL, 0);

    int proc_count = size / sizeof(struct kinfo_proc);
    kinfo_proc* proc_list = new kinfo_proc[proc_count];

    sysctl(mib, 4, proc_list, &size, NULL, 0);

    std::set<ProcessInfo> result;
    for (int i = 0; i < proc_count; i++) {
        result.emplace(ProcessInfo { proc_list[i].kp_proc.p_pid, proc_list[i].kp_proc.p_comm });
    }

    delete[] proc_list;

#ifdef MAA_DEBUG
    LogInfo << "Process list:" << result;
#endif

    return result;
}

os_string get_process_path(os_pid pid)
{
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE] = { 0 };
    proc_pidpath(pid, pathbuf, sizeof(pathbuf));
    return pathbuf;
}

MAA_NS_END

#endif
