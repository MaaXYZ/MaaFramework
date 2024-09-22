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
    LogTrace << VAR(id) << VAR_VOIDP(tasker);
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

std::shared_ptr<const Context> Context::getptr() const
{
    return shared_from_this();
}

Context::Context(const Context& other)
    : std::enable_shared_from_this<Context>(other)
    , task_id_(other.task_id_)
    , tasker_(other.tasker_)
    , pipeline_override_(other.pipeline_override_)
// don't copy clone_holder_
{
    LogTrace << VAR(other.getptr());
}

MaaTaskId Context::run_pipeline(const std::string& entry, const json::value& pipeline_override)
{
    LogFunc << VAR(getptr()) << VAR(entry) << VAR(pipeline_override);

    PipelineTask task(entry, tasker_, getptr());
    task.override_pipeline(pipeline_override);
    task.run();
    return task.task_id();
}

MaaRecoId Context::run_recognition(const std::string& entry, const json::value& pipeline_override, const cv::Mat& image)
{
    LogFunc << VAR(getptr()) << VAR(entry) << VAR(pipeline_override);

    RecognitionTask task(entry, tasker_, getptr());
    task.override_pipeline(pipeline_override);
    return task.run_with_param(image);
}

MaaNodeId
    Context::run_action(const std::string& entry, const json::value& pipeline_override, const cv::Rect& box, const std::string& reco_detail)
{
    LogFunc << VAR(getptr()) << VAR(entry) << VAR(pipeline_override) << VAR(box) << VAR(reco_detail);

    ActionTask task(entry, tasker_, getptr());
    task.override_pipeline(pipeline_override);
    json::value j_detail = json::parse(reco_detail).value_or(reco_detail);
    return task.run_with_param(box, j_detail);
}

bool Context::override_pipeline(const json::value& pipeline_override)
{
    LogFunc << VAR(getptr()) << VAR(pipeline_override);

    if (!pipeline_override.is_object()) {
        LogError << "json is not object";
        return false;
    }

    if (!tasker_) {
        LogError << "tasker is null";
        return false;
    }
    auto* resource = tasker_->resource();
    if (!resource) {
        LogError << "resource not binded";
        return false;
    }
    auto& default_mgr = resource->default_pipeline();

    for (const auto& [key, value] : pipeline_override.as_object()) {
        PipelineData result;
        auto default_result = get_pipeline_data(key).value_or(default_mgr.get_pipeline());
        bool ret = MAA_RES_NS::PipelineResMgr::parse_task(key, value, result, default_result, default_mgr);
        if (!ret) {
            LogError << "parse_task failed" << VAR(key) << VAR(value);
            return false;
        }

        pipeline_override_.insert_or_assign(key, std::move(result));
    }

    return check_pipeline();
}

bool Context::override_next(const std::string& name, const std::vector<std::string>& next)
{
    LogFunc << VAR(getptr()) << VAR(name) << VAR(next);

    auto data_opt = get_pipeline_data(name);
    if (!data_opt) {
        LogError << "get_pipeline_data failed" << VAR(name);
        return false;
    }

    data_opt->next = next;

    pipeline_override_.insert_or_assign(name, std::move(*data_opt));

    return check_pipeline();
}

Context* Context::clone() const
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

std::optional<Context::PipelineData> Context::get_pipeline_data(const std::string& task_name)
{
    auto override_it = pipeline_override_.find(task_name);
    if (override_it != pipeline_override_.end()) {
        return override_it->second;
    }

    if (!tasker_) {
        LogError << "tasker is null";
        return std::nullopt;
    }
    auto* resource = tasker_->resource();
    if (!resource) {
        LogError << "resource not binded";
        return std::nullopt;
    }

    auto& raw_data_map = resource->pipeline_res().get_pipeline_data_map();
    auto raw_it = raw_data_map.find(task_name);
    if (raw_it != raw_data_map.end()) {
        return raw_it->second;
    }

    LogError << "task not found" << VAR(task_name);
    return std::nullopt;
}

bool Context::check_pipeline() const
{
    if (!tasker_) {
        LogError << "tasker is null";
        return false;
    }
    auto* resource = tasker_->resource();
    if (!resource) {
        LogError << "resource not binded";
        return false;
    }

    auto raw = resource->pipeline_res().get_pipeline_data_map();
    auto all = pipeline_override_;
    all.merge(raw);

    return MAA_RES_NS::PipelineResMgr::check_all_next_list(all);
}

MAA_TASK_NS_END
