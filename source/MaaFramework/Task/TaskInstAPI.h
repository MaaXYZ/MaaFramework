#pragma once

#include "Conf/Conf.h"

#include "Instance/InstanceInternalAPI.hpp"
#include "Task/TaskDataMgr.h"

MAA_TASK_NS_BEGIN

class TaskInstAPI
{
public:
    virtual InstanceInternalAPI* inst() = 0;
    virtual TaskDataMgr& data_mgr() = 0;
};

MAA_TASK_NS_END
