#include "RecognitionTask.h"

#include "Controller/ControllerAgent.h"
#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/Logger.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

MaaRecoId RecognitionTask::run_with_param(const cv::Mat& image)
{
    LogFunc << VAR(entry_);

    if (!context_) {
        LogError << "context is null";
        return MaaInvalidId;
    }

    auto node_opt = context_->get_pipeline_data(entry_);
    if (!node_opt) {
        LogError << "get_pipeline_data failed, task not exist" << VAR(entry_);
        return MaaInvalidId;
    }
    const auto& node = *node_opt;

    auto reco = run_recognition(image, node);

    NodeDetail result {
        .node_id = generate_node_id(),
        .name = entry_,
        .reco_id = reco.reco_id,
        .action_id = MaaInvalidId,
        .completed = reco.box.has_value(),
    };
    set_node_detail(result.node_id, result, node.focus);

    return reco.reco_id;
}

MAA_TASK_NS_END
