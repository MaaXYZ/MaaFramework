#pragma once

#include "TaskBase.h"

#include "Common/Conf.h"

MAA_TASK_NS_BEGIN

class RecognitionTask : public TaskBase
{
public:
    RecognitionTask(const cv::Mat& image, std::string entry, Tasker* tasker, std::shared_ptr<Context> context = nullptr);

    virtual ~RecognitionTask() override = default;

public:
    virtual bool run() override;

public:
    MaaRecoId run_impl();

private:
    cv::Mat image_;
};

MAA_TASK_NS_END
