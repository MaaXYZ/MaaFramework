#pragma once

#include "TaskBase.h"

#include <optional>
#include <set>

#include "Common/Conf.h"
#include "Vision/OCRer.h"

MAA_RES_NS_BEGIN
struct NodeAttr;
MAA_RES_NS_END

MAA_TASK_NS_BEGIN

struct BatchableOCRInfo
{
    std::string model;
    bool only_rec = false;
    std::set<std::string> node_names;
};

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
    std::optional<BatchableOCRInfo> collect_batchable_ocr_nodes(const std::vector<MAA_RES_NS::NodeAttr>& list);
    void save_on_error(const std::string& node_name);
};

MAA_TASK_NS_END
