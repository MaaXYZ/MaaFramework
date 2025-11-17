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
    NodeDetail run_next(const PipelineData::NextList& list, const PipelineData& pretask);
    RecoResult recognize_list(const PipelineData& cur_node, const cv::Mat& image, const PipelineData::NextList& list);
};

MAA_TASK_NS_END
