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

TaskBase::NextIter TaskBase::run_recogintion(const cv::Mat& image, const PipelineData::NextList& list, HitDetail& hit_detail)
{
    const NextIter NotFound = list.cend();

    LogFunc << VAR(pre_hit_task_) << VAR(list);

    if (image.empty()) {
        LogError << "Image is empty";
        return NotFound;
    }

    if (debug_mode()) {
        json::value detail = basic_info()
                             | json::object {
                                   { "list", json::array(list) },
                               };
        notify(MaaMsg_Task_Debug_ListToRecognize, detail);
    }

    bool hit = false;

    Recognizer recognizer(tasker_, context_, image);

    auto iter = list.cbegin();

    for (; iter != list.cend(); ++iter) {
        const std::string& name = iter->name;
        uint64_t& hit_times = hit_times_map_[name];

        const auto& pipeline_data = context_.get_pipeline_data(name);
        if (!pipeline_data.enabled || pipeline_data.hit_limit <= hit_times) {
            LogDebug << "Task disabled or hit over limit" << name << VAR(pipeline_data.enabled) << VAR(hit_times)
                     << VAR(pipeline_data.hit_limit);
            continue;
        }

        auto reco = recognizer.recognize(pipeline_data);

        if (debug_mode()) {
            json::value cb_detail = basic_info() | reco_result_to_json(name, reco);
            notify(MaaMsg_Task_Debug_RecognitionResult, cb_detail);
        }

        if (!reco.hit) {
            continue;
        }

        hit = true;
        ++hit_times;
        hit_detail = HitDetail { .reco_uid = reco.uid,
                                 .reco_hit = *std::move(reco.hit),
                                 .reco_detail = std::move(reco.detail),
                                 .pipeline_data = pipeline_data };
        break;
    }

    if (!hit) {
        if (debug_mode()) {
            json::value detail = basic_info()
                                 | json::object {
                                       { "list", json::array(list) },
                                   };
            notify(MaaMsg_Task_Debug_MissAll, detail);
        }

        return NotFound;
    }

    if (debug_mode()) {
        json::value cb_detail = basic_info() | hit_detail_to_json(hit_detail);
        notify(MaaMsg_Task_Debug_Hit, cb_detail);
    }

    return iter;
}

bool TaskBase::run_action(const HitDetail& hit)
{
    Actuator actuator(tasker_, context_);

    const std::string& name = hit.pipeline_data.name;

    NodeDetail node_detail { .name = name, .hit = hit };
    MaaNodeId node_id = ++s_global_node_id;

    if (debug_mode() || hit.pipeline_data.focus) {
        json::value cb_detail = basic_info() | node_detail_to_json(node_id, node_detail);
        if (debug_mode()) {
            notify(MaaMsg_Task_Debug_ReadyToRun, cb_detail);
        }
        if (hit.pipeline_data.focus) {
            notify(MaaMsg_Task_Focus_ReadyToRun, cb_detail);
        }
    }

    bool ret = actuator.run(hit.reco_hit, hit.reco_detail, hit.pipeline_data);

    node_detail.completed = ret;
    add_node_detail(node_id, node_detail);

    if (debug_mode() || hit.pipeline_data.focus) {
        json::value cb_detail = basic_info() | node_detail_to_json(node_id, node_detail);
        if (debug_mode()) {
            notify(MaaMsg_Task_Debug_Completed, cb_detail);
        }
        if (hit.pipeline_data.focus) {
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

json::object TaskBase::hit_detail_to_json(const HitDetail& detail)
{
    return {
        { "name", detail.pipeline_data.name },
        { "recognition",
          {
              { "id", detail.reco_uid },
              { "box", detail.reco_hit },
              { "detail", detail.reco_detail },
              { "hit_task", true },
          } },
    };
}

json::object TaskBase::node_detail_to_json(MaaNodeId node_id, const NodeDetail& detail)
{
    return hit_detail_to_json(detail.hit)
           | json::object {
                 { "node_id", node_id },
                 { "completed", detail.completed },
             };
}

MAA_TASK_NS_END
