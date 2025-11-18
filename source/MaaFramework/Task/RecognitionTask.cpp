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
    LogFunc << VAR(entry_) << VAR(task_id_);

    if (!context_) {
        LogError << "context is null";
        return MaaInvalidId;
    }

    auto node_opt = context_->get_pipeline_data(entry_);
    if (!node_opt) {
        LogError << "get_pipeline_data failed, task not exist" << VAR(entry_);
        return MaaInvalidId;
    }

    const auto& cur_node = *node_opt;

    auto node_id = generate_node_id();

    const json::value node_cb_detail {
        { "task_id", task_id() },
        { "node_id", node_id },
        { "name", entry_ },
        { "focus", cur_node.focus },
    };

    if (debug_mode() || !cur_node.focus.is_null()) {
        notify(MaaMsg_Node_RecognitionNode_Starting, node_cb_detail);
    }

    auto reco = run_recognition(image, cur_node);

    bool hit = reco.box.has_value();
    NodeDetail result {
        .node_id = node_id,
        .name = entry_,
        .reco_id = reco.reco_id,
        .action_id = MaaInvalidId,
        .completed = hit,
    };
    LogInfo << "RecognitionTask node done" << VAR(result) << VAR(task_id_);
    set_node_detail(result.node_id, result);

    if (debug_mode() || !cur_node.focus.is_null()) {
        notify(hit ? MaaMsg_Node_RecognitionNode_Succeeded : MaaMsg_Node_RecognitionNode_Failed, node_cb_detail);
    }

    return reco.reco_id;
}

MAA_TASK_NS_END
