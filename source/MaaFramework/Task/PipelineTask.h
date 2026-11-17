#pragma once

#include "TaskBase.h"

#include "Common/Conf.h"
#include "Vision/OCRer.h"

MAA_RES_NS_BEGIN
struct NodeAttr;
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
    NodeDetail run_next(const std::vector<MAA_RES_NS::NodeAttr>& next, const PipelineData& pretask);
    RecoResult recognize_list(const cv::Mat& image, const std::vector<MAA_RES_NS::NodeAttr>& list);
    void save_on_error(const std::string& node_name);

    inline static std::atomic<MaaId> next_list_id_ = 0;
    MaaNodeId source_node_id_ = MaaInvalidId;
    MaaNodeId active_node_id_ = MaaInvalidId;
    size_t round_ = 0;
    std::string list_type_ = "entry";
};

MAA_TASK_NS_END
