#include "TaskBase.h"

#include "Component/Actuator.h"
#include "Component/Recognizer.h"
#include "Controller/ControllerAgent.h"
#include "Global/OptionMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/Logger.h"
#include "Resource/ResourceMgr.h"

MAA_TASK_NS_BEGIN

TaskBase::TaskBase(std::string entry, Tasker* tasker, std::shared_ptr<Context> context)
    : tasker_(tasker)
    , entry_(std::move(entry))
    , cur_node_(entry_)
    , context_(context ? std::move(context) : Context::create(task_id_, tasker))
{
}

bool TaskBase::override_pipeline(const json::value& pipeline_override)
{
    return context_ && context_->override_pipeline(pipeline_override);
}

Tasker* TaskBase::tasker() const
{
    return tasker_;
}

MaaTaskId TaskBase::task_id() const
{
    return task_id_;
}

const std::string& TaskBase::entry() const
{
    return entry_;
}

MAA_RES_NS::ResourceMgr* TaskBase::resource()
{
    return tasker_ ? tasker_->resource() : nullptr;
}

MAA_CTRL_NS::ControllerAgent* TaskBase::controller()
{
    return tasker_ ? tasker_->controller() : nullptr;
}

RecoResult TaskBase::run_recognition(const cv::Mat& image, const PipelineData& data)
{
    LogFunc << VAR(cur_node_) << VAR(data.name);

    if (!context_) {
        LogError << "context is null";
        return {};
    }

    if (image.empty()) {
        LogError << "Image is empty";
        return {};
    }

    if (!data.enabled) {
        LogDebug << "node disabled" << data.name << VAR(data.enabled);
        return {};
    }

    size_t current_hit = context_->get_hit_count(data.name);
    if (current_hit >= static_cast<size_t>(data.max_hit)) {
        LogDebug << "max_hit reached" << VAR(data.name) << VAR(current_hit) << VAR(data.max_hit);
        return {};
    }

    Recognizer recognizer(tasker_, *context_, image);

    json::value cb_detail {
        { "task_id", task_id() },
        { "reco_id", recognizer.get_id() },
        { "name", data.name },
        { "focus", data.focus },
    };

    notify(MaaMsg_Node_Recognition_Starting, cb_detail);

    RecoResult result = recognizer.recognize(data);

    cb_detail["reco_details"] = result;
    notify(result.box ? MaaMsg_Node_Recognition_Succeeded : MaaMsg_Node_Recognition_Failed, cb_detail);

    if (result.box) {
        LogInfo << "reco hit" << VAR(result.name) << VAR(result.box);
        context_->increment_hit_count(data.name);
    }

    return result;
}

ActionResult TaskBase::run_action(const RecoResult& reco, const PipelineData& data)
{
    if (!context_) {
        LogError << "context is null";
        return {};
    }

    if (!reco.box) {
        LogError << "reco box is nullopt";
        return {};
    }

    if (!data.enabled) {
        LogDebug << "node disabled" << data.name << VAR(data.enabled);
        return {};
    }

    Actuator actuator(tasker_, *context_);
    json::value cb_detail {
        { "task_id", task_id() },
        { "action_id", actuator.get_id() },
        { "name", reco.name },
        { "focus", data.focus },
    };
    notify(MaaMsg_Node_Action_Starting, cb_detail);

    ActionResult result = actuator.run(*reco.box, reco.reco_id, data, entry_);

    cb_detail["action_details"] = result;
    notify(result.success ? MaaMsg_Node_Action_Succeeded : MaaMsg_Node_Action_Failed, cb_detail);

    return result;
}

cv::Mat TaskBase::screencap()
{
    if (!controller()) {
        return {};
    }

    return controller()->screencap();
}

MaaNodeId TaskBase::generate_node_id()
{
    return ++s_global_node_id;
}

void TaskBase::set_node_detail(MaaNodeId node_id, NodeDetail detail)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return;
    }

    auto& cache = tasker_->runtime_cache();
    cache.set_node_detail(node_id, detail);
    if (!detail.name.empty()) {
        cache.set_latest_node(detail.name, node_id);
    }

    // value_or 的默认值用于 run 到一半调用方手动 clear cache 了的情况
    TaskDetail task_detail =
        cache.get_task_detail(task_id_).value_or(TaskDetail { .task_id = task_id_, .entry = entry_, .status = MaaStatus_Running });
    task_detail.node_ids.emplace_back(node_id);

    set_task_detail(task_detail);
}

void TaskBase::set_task_detail(TaskDetail detail)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return;
    }

    auto& cache = tasker_->runtime_cache();
    cache.set_task_detail(task_id_, detail);
}

bool TaskBase::debug_mode() const
{
    return MAA_GLOBAL_NS::OptionMgr::get_instance().debug_mode();
}

void TaskBase::notify(std::string_view msg, const json::value detail)
{
    if (!tasker_ || !context_) {
        return;
    }

    tasker_->context_notify(context_.get(), msg, detail);
}

MAA_TASK_NS_END
