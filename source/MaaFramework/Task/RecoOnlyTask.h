#pragma once

#include "TaskBase.h"

#include "Common/Conf.h"

MAA_TASK_NS_BEGIN

class RecoOnlyTask : public TaskBase
{
public:
    using TaskBase::TaskBase;

    virtual ~RecoOnlyTask() override = default;

public:
    MaaRecoId run_with_param(const cv::Mat& image);
};

MAA_TASK_NS_END

