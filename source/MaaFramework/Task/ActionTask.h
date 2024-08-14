#pragma once

#include "TaskBase.h"

MAA_TASK_NS_BEGIN

class ActionTask : public TaskBase
{
public:
    ActionTask(std::string entry, Tasker* tasker, PipelineDataMap pp_override = {});
    virtual ~ActionTask() = default;

    virtual bool run() override;

    virtual void post_stop() override {}
};

MAA_TASK_NS_END
