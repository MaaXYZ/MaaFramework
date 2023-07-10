#pragma once
#pragma once

#include "Resource/PipelineTypes.h"
#include "TaskBase.h"

MAA_TASK_NS_BEGIN

class CustomTask : public TaskBase
{
public:
    CustomTask(std::string task_name, MaaCustomTaskHandle task_handle, InstanceInternalAPI* inst)
        : TaskBase(task_name, inst), task_handle_(task_handle)
    {}
    virtual ~CustomTask() override = default;

    virtual bool run() override;
    virtual bool set_param(const json::value& param) override;

private:
    MaaCustomTaskHandle task_handle_;
};

using CustomTaskPtr = std::shared_ptr<CustomTask>;

MAA_TASK_NS_END
