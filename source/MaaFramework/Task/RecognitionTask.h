#pragma once

#include "TaskBase.h"

MAA_TASK_NS_BEGIN

class RecognitionTask : public TaskBase
{
public:
    using PipelineDataMap = Context::PipelineDataMap;

public:
    RecognitionTask(std::string entry, Tasker* tasker, PipelineDataMap pp_override);
    virtual ~RecognitionTask() = default;

    virtual bool run() override;

    virtual void post_stop() override {}

public:
    bool run_with_param(const cv::Mat& image);
};

MAA_TASK_NS_END
