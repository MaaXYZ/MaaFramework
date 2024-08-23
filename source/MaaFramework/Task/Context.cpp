#include "Context.h"

#include <meojson/json.hpp>

#include "ActionTask.h"
#include "PipelineTask.h"
#include "RecognitionTask.h"
#include "Tasker/Tasker.h"
#include "Utils/Logger.h"

MAA_TASK_NS_BEGIN

Context::Context(MaaTaskId id, Tasker* tasker, PrivateArg)
    : task_id_(id)
    , tasker_(tasker)
{
}

std::shared_ptr<Context> Context::create(MaaTaskId id, Tasker* tasker)
{
    LogTrace << VAR(id) << VAR_VOIDP(tasker);

    return std::make_shared<Context>(id, tasker, PrivateArg {});
}

std::shared_ptr<Context> Context::getptr()
{
    return shared_from_this();
}

MaaTaskId Context::run_pipeline(const std::string& entry, const json::value& pipeline_override)
{
    LogFunc << VAR(getptr());

    PipelineTask task(entry, tasker_, getptr());
    task.override_pipeline(pipeline_override);
    task.run();
    return task.task_id();
}

MaaTaskId Context::run_recognition(const std::string& entry, const json::value& pipeline_override, const cv::Mat& image)
{
    LogFunc << VAR(getptr());

    RecognitionTask task(entry, tasker_, getptr());
    task.override_pipeline(pipeline_override);
    task.run_with_param(image);
    return task.task_id();
}

MaaTaskId
    Context::run_action(const std::string& entry, const json::value& pipeline_override, const cv::Rect& box, const std::string& reco_detail)
{
    LogFunc << VAR(getptr());

    ActionTask task(entry, tasker_, getptr());
    task.override_pipeline(pipeline_override);
    json::value j_detail = json::parse(reco_detail).value_or(reco_detail);
    task.run_with_param(box, j_detail);
    return task.task_id();
}

bool Context::override_pipeline(const json::value& pipeline_override)
{
    LogFunc << VAR(getptr());

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
        LogError << "Parse pipeline failed";
        return false;
    }

    new_override.merge(std::move(pipeline_override_));
    pipeline_override_ = std::move(new_override);

    return true;
}

Context* Context::clone()
{
    LogFunc << VAR(getptr());

    auto cloned = std::make_shared<Context>(*this);
    auto& ref = clone_holder_.emplace_back(std::move(cloned));
    LogTrace << VAR(getptr()) << VAR(ref);

    return ref.get();
}

MaaTaskId Context::task_id() const
{
    return task_id_;
}

Tasker* Context::tasker() const
{
    return tasker_;
}

Context::PipelineData Context::get_pipeline_data(const std::string& task_name)
{
    auto override_it = pipeline_override_.find(task_name);
    if (override_it == pipeline_override_.end()) {
        if (!tasker_) {
            LogError << "tasker is null";
            return {};
        }
        if (!tasker_->resource()) {
            LogError << "resource not binded";
            return {};
        }
        auto& raw_data_mgr = tasker_->resource()->pipeline_res();

        return raw_data_mgr.get_pipeline_data(task_name);
    }

    return override_it->second;
}

MAA_TASK_NS_END
