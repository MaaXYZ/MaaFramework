#pragma once

#include "TaskBase.h"

#include "Common/Conf.h"

MAA_TASK_NS_BEGIN

class PipelineTask : public TaskBase
{
public:
    using TaskBase::TaskBase;

    virtual ~PipelineTask() override = default;

    virtual bool run() override;
    virtual void post_stop() override;

private:
    NodeDetail run_next(const std::vector<std::string>& next, const PipelineData& pretask);
    RecoResult recognize_list(const cv::Mat& image, const std::vector<std::string>& list);
};

MAA_TASK_NS_END
