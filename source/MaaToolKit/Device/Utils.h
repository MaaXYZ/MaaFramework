#pragma once

#include "Conf/Conf.h"
#include "Utils/Platform.hpp"

#include <functional>

MAA_TOOLKIT_DEVICE_NS_BEGIN

struct ProcessInfo
{
    os_pid pid;
    os_string name;
};

std::vector<ProcessInfo> get_processes_info();
os_string get_process_path(os_pid pid);

MAA_TOOLKIT_DEVICE_NS_END
