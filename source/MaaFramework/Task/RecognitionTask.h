#pragma once

#include "TaskBase.h"

MAA_TASK_NS_BEGIN

class RecognitionTask : public TaskBase
{
public:
    using TaskBase::TaskBase;

    virtual ~RecognitionTask() override = default;

    virtual bool run() override;

    virtual void post_stop() override {}

public:
    MaaRecoId run_with_param(const cv::Mat& image);
};

MAA_TASK_NS_END
