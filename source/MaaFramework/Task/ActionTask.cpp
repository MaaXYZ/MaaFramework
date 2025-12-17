#include "ActionTask.h"

#include "Controller/ControllerAgent.h"
#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/Logger.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/Tasker.h"
#include "Vision/VisionBase.h"

MAA_TASK_NS_BEGIN

ActionTask::ActionTask(
    const cv::Rect& box,
    const std::string& reco_detail,
    std::string entry,
    Tasker* tasker,
    std::shared_ptr<Context> context)
    : TaskBase(std::move(entry), tasker, std::move(context))
    , box_(box)
    , reco_detail_(json::parse(reco_detail).value_or(reco_detail))
{
}

bool ActionTask::run()
{
    return run_impl() != MaaInvalidId;
}

MaaActId ActionTask::run_impl()
{
    LogFunc << VAR(entry_) << VAR(task_id_);

    if (!tasker_) {
        LogError << "tasker is null";
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

    notify(MaaMsg_Node_ActionNode_Starting, node_cb_detail);

    RecoResult fake_reco {
        .reco_id = MaaInvalidId,
        .box = box_,
        .detail = reco_detail_,
    };

    auto act = run_action(fake_reco, cur_node);

    NodeDetail result {
        .node_id = node_id,
        .name = entry_,
        .reco_id = fake_reco.reco_id,
        .action_id = act.action_id,
        .completed = act.success,
    };
    LogInfo << "ActionTask node done" << VAR(result) << VAR(task_id_);
    set_node_detail(result.node_id, result);

    node_cb_detail["node_details"] = result;
    node_cb_detail["reco_details"] = fake_reco;
    node_cb_detail["action_details"] = act;
    notify(act.success ? MaaMsg_Node_ActionNode_Succeeded : MaaMsg_Node_ActionNode_Failed, node_cb_detail);

    return act.action_id;
}

MAA_TASK_NS_END
