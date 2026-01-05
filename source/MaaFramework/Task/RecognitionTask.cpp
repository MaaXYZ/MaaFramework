#include "RecognitionTask.h"

#include "Controller/ControllerAgent.h"
#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/Logger.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

RecognitionTask::RecognitionTask(const cv::Mat& image, std::string entry, Tasker* tasker, std::shared_ptr<Context> context)
    : TaskBase(std::move(entry), tasker, std::move(context))
    , image_(image)
{
}

bool RecognitionTask::run()
{
    return run_impl() != MaaInvalidId;
}

MaaRecoId RecognitionTask::run_impl()
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

    json::value node_cb_detail {
        { "task_id", task_id() },
        { "node_id", node_id },
        { "name", entry_ },
        { "focus", cur_node.focus },
    };

    notify(MaaMsg_Node_RecognitionNode_Starting, node_cb_detail);

    auto reco = run_recognition(image_, cur_node);

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

    node_cb_detail["node_details"] = result;
    node_cb_detail["reco_details"] = reco;
    node_cb_detail["action_details"] = nullptr;
    notify(hit ? MaaMsg_Node_RecognitionNode_Succeeded : MaaMsg_Node_RecognitionNode_Failed, node_cb_detail);

    return reco.reco_id;
}

MAA_TASK_NS_END
