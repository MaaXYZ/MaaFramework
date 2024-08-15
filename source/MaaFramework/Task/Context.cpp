#include "Context.h"

#include <meojson/json.hpp>

#include "ActionTask.h"
#include "PipelineTask.h"
#include "RecognitionTask.h"
#include "Tasker/Tasker.h"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

Context::Context(MaaTaskId id, Tasker* tasker, PipelineDataMap pp_override)
    : task_id_(id)
    , tasker_(tasker)
    , pipeline_override_(std::move(pp_override))
{
}

MaaTaskId Context::run_pipeline(const std::string& entry, const json::value& pipeline_override)
{
    PipelineTask task(entry, tasker_, pipeline_override_);
    task.override_pipeline(pipeline_override);
    task.run();
    return task.task_id();
}

MaaTaskId Context::run_recognition(const std::string& entry, const json::value& pipeline_override, const cv::Mat& image)
{
    RecognitionTask task(entry, tasker_, pipeline_override_);
    task.override_pipeline(pipeline_override);
    task.run_with_param(image);
    return task.task_id();
}

MaaTaskId
    Context::run_action(const std::string& entry, const json::value& pipeline_override, const cv::Rect& box, const std::string& reco_detail)
{
    ActionTask task(entry, tasker_, pipeline_override_);
    task.override_pipeline(pipeline_override);
    json::value j_detail = json::parse(reco_detail).value_or(reco_detail);
    task.run_with_param(box, j_detail);
    return task.task_id();
}

bool Context::override_pipeline(const json::value& pipeline_override)
{
    LogFunc << VAR(pipeline_override);

    if (!tasker_) {
        LogError << "tasker is null";
        return {};
    }
    if (!tasker_->resource()) {
        LogError << "resource not binded";
        return {};
    }

    MAA_RES_NS::PipelineResMgr::PipelineDataMap new_override;
    auto& raw_data_map = tasker_->resource()->pipeline_res().get_pipeline_data_map();
    std::set<std::string> existing_keys;
    bool parsed = MAA_RES_NS::PipelineResMgr::parse_config(pipeline_override, new_override, existing_keys, raw_data_map);
    if (!parsed) {
        LogError << "Parse json failed";
        return false;
    }

    new_override.merge(std::move(pipeline_override_));
    pipeline_override_ = std::move(new_override);

    return true;
}

MaaTaskId Context::task_id() const
{
    return task_id_;
}

Tasker* Context::tasker()
{
    return tasker_;
}

Context::PipelineData Context::get_pipeline_data(const std::string& task_name)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return {};
    }
    if (!tasker_->resource()) {
        LogError << "resource not binded";
        return {};
    }
    auto& raw_data_mgr = tasker_->resource()->pipeline_res();

    auto override_it = pipeline_override_.find(task_name);
    if (override_it == pipeline_override_.end()) {
        return raw_data_mgr.get_pipeline_data(task_name);
    }

    return override_it->second;
}

MAA_TASK_NS_END
