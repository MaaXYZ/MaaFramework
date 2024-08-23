#include "TaskBase.h"

#include "Component/Actuator.h"
#include "Component/Recognizer.h"
#include "Controller/ControllerAgent.h"
#include "Global/GlobalOptionMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "Resource/ResourceMgr.h"
#include "Utils/JsonExt.hpp"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

TaskBase::TaskBase(std::string entry, Tasker* tasker)
    : tasker_(tasker)
    , entry_(std::move(entry))
    , cur_task_(entry_)
    , context_(Context::create(task_id_, tasker))
{
}

TaskBase::TaskBase(std::string entry, Tasker* tasker, std::shared_ptr<Context> context)
    : tasker_(tasker)
    , entry_(std::move(entry))
    , cur_task_(entry_)
    , context_(std::move(context))
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

void TaskBase::notify(std::string_view msg, json::value detail)
{
    if (tasker_) {
        tasker_->notify(msg, detail);
    }
}

RecoResult TaskBase::run_recogintion(const cv::Mat& image, const PipelineData::NextList& list)
{
    LogFunc << VAR(cur_task_) << VAR(list);

    if (!context_) {
        LogError << "context is null";
        return {};
    }

    if (image.empty()) {
        LogError << "Image is empty";
        return {};
    }

    if (list.empty()) {
        return {};
    }

    if (debug_mode()) {
        json::value cb_detail = basic_info() | json::object { { "list", json::array(list) } };
        notify(MaaMsg_Task_Debug_ListToRecognize, cb_detail);
    }

    Recognizer recognizer(tasker_, *context_, image);

    for (const auto& name : list) {
        const auto& pipeline_data = context_->get_pipeline_data(name);

        if (!pipeline_data.enabled) {
            LogDebug << "Task disabled or times over limit" << name << VAR(pipeline_data.enabled);
            continue;
        }

        RecoResult result = recognizer.recognize(pipeline_data);

        if (debug_mode()) {
            json::value cb_detail = basic_info() | reco_detail_to_json(result);
            notify(MaaMsg_Task_Debug_RecognitionResult, cb_detail);
        }

        if (!result.box) {
            continue;
        }

        LogInfo << "Task hit" << VAR(result.name) << VAR(result.box);

        if (debug_mode()) {
            json::value cb_detail = basic_info() | reco_detail_to_json(result);
            notify(MaaMsg_Task_Debug_Hit, cb_detail);
        }

        return result;
    }

    if (debug_mode()) {
        json::value cb_detail = basic_info() | json::object { { "list", json::array(list) } };
        notify(MaaMsg_Task_Debug_MissAll, cb_detail);
    }

    return {};
}

NodeDetail TaskBase::run_action(const RecoResult& reco)
{
    if (!context_) {
        LogError << "context is null";
        return {};
    }

    if (!reco.box) {
        LogError << "reco box is nullopt, can NOT run";
        return {};
    }

    uint64_t& times = context_->action_times()[reco.name];
    ++times;

    NodeDetail result {
        .node_id = generate_node_id(),
        .name = reco.name,
        .reco_id = reco.reco_id,
        .times = times,
    };

    const auto& pipeline_data = context_->get_pipeline_data(reco.name);

    json::value cb_detail = basic_info() | node_detail_to_json(result);
    if (debug_mode()) {
        notify(MaaMsg_Task_Debug_ReadyToRun, cb_detail);
    }
    if (pipeline_data.focus) {
        notify(MaaMsg_Task_Focus_ReadyToRun, cb_detail);
    }

    Actuator actuator(tasker_, *context_);
    result.completed = actuator.run(*reco.box, reco.detail, pipeline_data);

    add_node_detail(result.node_id, result);

    cb_detail = basic_info() | node_detail_to_json(result);
    if (debug_mode()) {
        notify(MaaMsg_Task_Debug_ReadyToRun, cb_detail);
    }
    if (pipeline_data.focus) {
        notify(MaaMsg_Task_Focus_ReadyToRun, cb_detail);
    }

    return result;
}

cv::Mat TaskBase::screencap()
{
    if (!controller()) {
        return {};
    }

    return controller()->screencap();
}

MaaTaskId TaskBase::generate_node_id()
{
    return ++s_global_node_id;
}

void TaskBase::add_node_detail(int64_t node_id, NodeDetail detail)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return;
    }

    auto& cache = tasker_->runtime_cache();
    cache.add_node_detail(node_id, detail);

    TaskDetail task_detail = cache.get_task_detail(task_id_).value_or(TaskDetail { .entry = entry_ });
    task_detail.node_ids.emplace_back(node_id);
    cache.add_task_detail(task_id_, task_detail);
}

bool TaskBase::debug_mode() const
{
    return GlobalOptionMgr::get_instance().debug_message();
}

json::object TaskBase::basic_info()
{
    return {
        { "task_id", task_id_ },
        { "entry", entry_ },
        { "hash", resource() ? resource()->get_hash() : std::string() },
        { "uuid", controller() ? controller()->get_uuid() : std::string() },
        { "current", cur_task_ },
    };
}

json::object TaskBase::reco_detail_to_json(const RecoResult& res)
{
    return { { "recognition",
               {
                   { "reco_id", res.reco_id },
                   { "name", res.name },
                   { "box", res.box ? json::value(*res.box) : json::value(nullptr) },
                   { "detail", res.detail },
               } } };
}

json::object TaskBase::node_detail_to_json(const NodeDetail& detail)
{
    return { { "node",
               {
                   { "node_id", detail.node_id },
                   { "name", detail.name },
                   { "reco_id", detail.reco_id },
                   { "times", detail.times },
                   { "completed", detail.completed },
               } } };
}

MAA_TASK_NS_END
