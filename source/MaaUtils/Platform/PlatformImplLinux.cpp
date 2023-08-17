#ifdef __linux__

#include "Platform.h"

#include <tuple>

#include <cstdlib>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

std::set<ProcessInfo> list_processes()
{
    // TODO
    return {};
}

os_string get_process_path(os_pid pid)
{
    // TODO
    std::ignore = pid;
    return {};
}

#endif
