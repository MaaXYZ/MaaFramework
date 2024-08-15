#pragma once

#include "TaskBase.h"

MAA_TASK_NS_BEGIN

class ActionTask : public TaskBase
{
public:
    using PipelineDataMap = Context::PipelineDataMap;

public:
    ActionTask(std::string entry, Tasker* tasker, PipelineDataMap pp_override);
    virtual ~ActionTask() = default;

    virtual bool run() override;

    virtual void post_stop() override {}

public:
    bool run_with_param(const cv::Rect& box, const json::value& reco_detail);
};

MAA_TASK_NS_END
