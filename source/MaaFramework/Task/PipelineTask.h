#pragma once

#include "TaskBase.h"

MAA_TASK_NS_BEGIN

class PipelineTask : public TaskBase
{
public:
    using PipelineDataMap = Context::PipelineDataMap;

public:
    using TaskBase::TaskBase;

    virtual ~PipelineTask() override = default;

    virtual bool run() override;
    virtual void post_stop() override;

private:
    NodeDetail run_reco_and_action(const PipelineData::NextList& list, const PipelineData& pretask);

private:
    bool need_to_stop_ = false;
};

MAA_TASK_NS_END
