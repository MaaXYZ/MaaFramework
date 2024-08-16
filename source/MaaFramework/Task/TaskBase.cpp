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

TaskBase::TaskBase(std::string entry, Tasker* tasker, PipelineDataMap pp_override)
    : tasker_(tasker)
    , entry_(std::move(entry))
    , context_(task_id_, tasker_, std::move(pp_override))
{
}

bool TaskBase::override_pipeline(const json::value& pipeline_override)
{
    return context_.override_pipeline(pipeline_override);
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
    LogFunc << VAR(pre_hit_task_) << VAR(list);

    if (image.empty()) {
        LogError << "Image is empty";
        return {};
    }

    if (debug_mode()) {
        json::value cb_detail = basic_info() | json::object { { "list", json::array(list) } };
        notify(MaaMsg_Task_Debug_ListToRecognize, cb_detail);
    }

    Recognizer recognizer(tasker_, context_, image);

    for (const auto& next_obj : list) {
        const std::string& name = next_obj.name;
        uint64_t& hit_times = hit_times_map_[name];

        const auto& pipeline_data = context_.get_pipeline_data(name);
        if (!pipeline_data.enabled || pipeline_data.hit_limit <= hit_times) {
            LogDebug << "Task disabled or hit over limit" << name << VAR(pipeline_data.enabled) << VAR(hit_times)
                     << VAR(pipeline_data.hit_limit);
            continue;
        }

        RecoResult result = recognizer.recognize(pipeline_data);

        if (debug_mode()) {
            json::value cb_detail = basic_info() | reco_result_to_json(name, result);
            notify(MaaMsg_Task_Debug_RecognitionResult, cb_detail);
        }

        if (!result.hit) {
            continue;
        }

        ++hit_times;

        if (debug_mode()) {
            json::value cb_detail = basic_info() | reco_result_to_json(name, result);
            notify(MaaMsg_Task_Debug_Hit, cb_detail);
        }
    }

    if (debug_mode()) {
        json::value cb_detail = basic_info() | json::object { { "list", json::array(list) } };
        notify(MaaMsg_Task_Debug_MissAll, cb_detail);
    }

    return {};
}

bool TaskBase::run_action(const RecoResult& reco)
{
    if (!reco.hit) {
        LogError << "reco hit is nullopt, can NOT run";
        return false;
    }

    const auto& pipeline_data = context_.get_pipeline_data(reco.name);

    MaaNodeId node_id = ++s_global_node_id;
    NodeDetail node_detail { .name = reco.name, .reco_uid = reco.uid };

    if (debug_mode() || pipeline_data.focus) {
        json::value cb_detail = basic_info() | node_detail_to_json(node_id, node_detail);
        if (debug_mode()) {
            notify(MaaMsg_Task_Debug_ReadyToRun, cb_detail);
        }
        if (pipeline_data.focus) {
            notify(MaaMsg_Task_Focus_ReadyToRun, cb_detail);
        }
    }

    Actuator actuator(tasker_, context_);
    bool ret = actuator.run(*reco.hit, reco.detail, pipeline_data);

    node_detail.action_completed = ret;
    add_node_detail(node_id, node_detail);

    if (debug_mode() || pipeline_data.focus) {
        json::value cb_detail = basic_info() | node_detail_to_json(node_id, node_detail);
        if (debug_mode()) {
            notify(MaaMsg_Task_Debug_Completed, cb_detail);
        }
        if (pipeline_data.focus) {
            notify(MaaMsg_Task_Focus_Completed, cb_detail);
        }
    }

    return ret;
}

cv::Mat TaskBase::screencap()
{
    if (!controller()) {
        return {};
    }

    return controller()->screencap();
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
        { "pre_hit_task", pre_hit_task_ },
    };
}

json::object TaskBase::reco_result_to_json(const std::string& name, const RecoResult& res)
{
    return {
        { "name", name },
        { "recognition",
          {
              { "id", res.uid },
              { "box", res.hit ? json::value(*res.hit) : json::value(nullptr) },
              { "detail", res.detail },
              { "hit_task", res.hit.has_value() },
          } },
    };
}

json::object TaskBase::node_detail_to_json(MaaNodeId node_id, const NodeDetail& detail)
{
    return {
        { "node_id", node_id },
        { "reco_id", detail.reco_uid },
        { "action_completed", detail.action_completed },
    };
}

MAA_TASK_NS_END
