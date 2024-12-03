#pragma once

#include "TaskBase.h"

MAA_TASK_NS_BEGIN

// for MaaTaskerPostStop, as a stop mark
class EmptyTask : public TaskBase
{
public:
    using TaskBase::TaskBase;

    virtual ~EmptyTask() override = default;

    virtual bool run() override { return true; }

    virtual void post_stop() override {}
};

MAA_TASK_NS_END
