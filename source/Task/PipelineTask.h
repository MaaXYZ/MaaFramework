#pragma once

#include "TaskBase.h"

MAA_TASK_NS_BEGIN

class PipelineTask : public TaskBase
{
public:
    using TaskBase::TaskBase;
    virtual ~PipelineTask() override = default;

    virtual bool run() override;
    virtual bool set_param(const json::value& param) override;

    virtual std::string_view type() const override { return "PipelineTask"; }

private:
    std::string begin_task_;
    std::string connecting_task_;
    std::string disconnected_task_;
};

MAA_TASK_NS_END
