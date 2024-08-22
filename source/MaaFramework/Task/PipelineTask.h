#pragma once

#include "TaskBase.h"

MAA_TASK_NS_BEGIN

class PipelineTask : public TaskBase
{
public:
    using PipelineDataMap = Context::PipelineDataMap;

public:
    PipelineTask(std::string entry, Tasker* tasker, PipelineDataMap pp_override);
    virtual ~PipelineTask() = default;

    virtual bool run() override;
    virtual void post_stop() override;

private:
    std::pair<NodeDetail, /* is breakpoint */ bool>
        run_reco_and_action(const PipelineData::NextList& next, const PipelineData::NextList& catch_next, const std::chrono::milliseconds& timeout);

private:
    bool need_to_stop_ = false;
};

MAA_TASK_NS_END
