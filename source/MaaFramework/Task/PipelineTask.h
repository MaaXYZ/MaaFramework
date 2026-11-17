#pragma once

#include "TaskBase.h"

#include <optional>
#include <set>

#include "Common/Conf.h"
#include "Vision/OCRTypes.h"

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
    struct BatchOCRPlan
    {
        std::set<std::string> owner_node_names;
        std::vector<MAA_VISION_NS::OCRerParam> entries;
    };

    struct OCRCollectContext
    {
        BatchOCRPlan plan;
        std::set<std::string> visiting_nodes;
    };

    NodeDetail run_next(const std::vector<MAA_RES_NS::NodeAttr>& next, const PipelineData& pretask);
    RecoResult recognize_list(const cv::Mat& image, const std::vector<MAA_RES_NS::NodeAttr>& list);
    std::optional<BatchOCRPlan> prepare_batch_ocr(const std::vector<MAA_RES_NS::NodeAttr>& list);
    void try_add_ocr_node(OCRCollectContext& ctx, const MAA_VISION_NS::OCRerParam& param);
    void collect_ocr_from_reco(OCRCollectContext& ctx, MAA_RES_NS::Recognition::Type type, const MAA_RES_NS::Recognition::Param& param);
    void collect_ocr_from_sub_recognitions(OCRCollectContext& ctx, const std::vector<MAA_RES_NS::Recognition::SubRecognition>& subs);

    void save_on_error(const std::string& node_name);

    inline static std::atomic<MaaId> next_list_id_ = 0;
    MaaNodeId source_node_id_ = MaaInvalidId;
    MaaNodeId active_node_id_ = MaaInvalidId;
    size_t round_ = 0;
    std::string list_type_ = "entry";
};

MAA_TASK_NS_END
