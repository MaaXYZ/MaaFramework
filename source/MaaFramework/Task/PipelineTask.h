#pragma once

#include "TaskBase.h"

#include "Common/Conf.h"

MAA_RES_NS_BEGIN
struct NodeWithAttr;
MAA_RES_NS_END

MAA_TASK_NS_BEGIN

class PipelineTask : public TaskBase
{
public:
    using TaskBase::TaskBase;

    virtual ~PipelineTask() override = default;

    virtual bool run() override;
    virtual void post_stop() override;

private:
    NodeDetail run_next(const std::vector<MAA_RES_NS::NodeWithAttr>& next, const PipelineData& pretask);
    RecoResult recognize_list(const cv::Mat& image, const std::vector<MAA_RES_NS::NodeWithAttr>& list);
};

MAA_TASK_NS_END
