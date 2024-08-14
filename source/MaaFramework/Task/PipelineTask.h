#pragma once

#include "TaskBase.h"

MAA_TASK_NS_BEGIN

class PipelineTask : public TaskBase
{
public:
    PipelineTask(std::string entry, Tasker* tasker, PipelineDataMap pp_override = {});
    virtual ~PipelineTask() = default;

    virtual bool run() override;
    virtual void post_stop() override;

private:
    NextIter run_reco_and_action(const PipelineData::NextList& list);

private:
    bool need_to_stop_ = false;
};

MAA_TASK_NS_END
