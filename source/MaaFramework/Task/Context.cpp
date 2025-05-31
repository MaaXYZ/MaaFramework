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
    LogDebug << VAR(id) << VAR_VOIDP(tasker);
}

std::shared_ptr<Context> Context::create(MaaTaskId id, Tasker* tasker)
{
    LogDebug << VAR(id) << VAR_VOIDP(tasker);

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
    LogDebug << VAR(other.getptr());
}

MaaTaskId Context::run_task(const std::string& entry, const json::object& pipeline_override)
{
    LogFunc << VAR(getptr()) << VAR(entry) << VAR(pipeline_override);

    if (!tasker_) {
        LogError << "tasker is null";
        return MaaInvalidId;
    }

    PipelineTask subtask(entry, tasker_, getptr());
    bool ov = subtask.override_pipeline(pipeline_override);
    if (!ov) {
        LogError << "failed to override_pipeline" << VAR(entry) << VAR(pipeline_override);
        return MaaInvalidId;
    }

    auto& runtime_cache = tasker_->runtime_cache();

    // context 的子任务没有 Pending 状态，直接就是 Running
    runtime_cache.set_task_detail(
        subtask.task_id(),
        MAA_TASK_NS::TaskDetail { .task_id = subtask.task_id(), .entry = subtask.entry(), .status = MaaStatus_Running });

    bool ret = subtask.run();

    {
        auto task_detail = runtime_cache.get_task_detail(subtask.task_id())
                               .value_or(MAA_TASK_NS::TaskDetail { .task_id = subtask.task_id(), .entry = entry });
        task_detail.status = ret ? MaaStatus_Succeeded : MaaStatus_Failed;
        runtime_cache.set_task_detail(subtask.task_id(), std::move(task_detail));
    }

    return subtask.task_id();
}

MaaRecoId Context::run_recognition(const std::string& entry, const json::object& pipeline_override, const cv::Mat& image)
{
    LogFunc << VAR(getptr()) << VAR(entry) << VAR(pipeline_override);

    RecognitionTask subtask(entry, tasker_, getptr());
    bool ov = subtask.override_pipeline(pipeline_override);
    if (!ov) {
        LogError << "failed to override_pipeline" << VAR(entry) << VAR(pipeline_override);
        return MaaInvalidId;
    }
    return subtask.run_with_param(image);
}

MaaNodeId Context::run_action(
    const std::string& entry,
    const json::object& pipeline_override,
    const cv::Rect& box,
    const std::string& reco_detail)
{
    LogFunc << VAR(getptr()) << VAR(entry) << VAR(pipeline_override) << VAR(box) << VAR(reco_detail);

    ActionTask subtask(entry, tasker_, getptr());
    bool ov = subtask.override_pipeline(pipeline_override);
    if (!ov) {
        LogError << "failed to override_pipeline" << VAR(entry) << VAR(pipeline_override);
        return MaaInvalidId;
    }
    json::value j_detail = json::parse(reco_detail).value_or(reco_detail);
    return subtask.run_with_param(box, j_detail);
}

bool Context::override_pipeline(const json::object& pipeline_override)
{
    LogFunc << VAR(getptr()) << VAR(pipeline_override);

    if (!tasker_) {
        LogError << "tasker is null";
        return false;
    }
    auto* resource = tasker_->resource();
    if (!resource) {
        LogError << "resource not bound";
        return false;
    }
    auto& default_mgr = resource->default_pipeline();

    for (const auto& [key, value] : pipeline_override) {
        PipelineData result;
        auto default_result = get_pipeline_data(key).value_or(default_mgr.get_pipeline());
        bool ret = MAA_RES_NS::PipelineResMgr::parse_node(key, value, result, default_result, default_mgr);
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
        LogError << "get_pipeline_data failed, task not exist" << VAR(name);
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
    LogDebug << VAR(getptr()) << VAR(ref);

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

std::optional<Context::PipelineData> Context::get_pipeline_data(const std::string& node_name)
{
    auto override_it = pipeline_override_.find(node_name);
    if (override_it != pipeline_override_.end()) {
        LogDebug << "found in override" << VAR(node_name);
        return override_it->second;
    }

    if (!tasker_) {
        LogError << "tasker is null";
        return std::nullopt;
    }
    auto* resource = tasker_->resource();
    if (!resource) {
        LogError << "resource not bound";
        return std::nullopt;
    }

    auto& raw_data_map = resource->pipeline_res().get_pipeline_data_map();
    auto raw_it = raw_data_map.find(node_name);
    if (raw_it != raw_data_map.end()) {
        return raw_it->second;
    }

    LogWarn << "task not found" << VAR(node_name);
    return std::nullopt;
}

bool& Context::need_to_stop()
{
    return need_to_stop_;
}

bool Context::check_pipeline() const
{
    if (!tasker_) {
        LogError << "tasker is null";
        return false;
    }
    auto* resource = tasker_->resource();
    if (!resource) {
        LogError << "resource not bound";
        return false;
    }

    auto raw = resource->pipeline_res().get_pipeline_data_map();
    auto all = pipeline_override_;
    all.merge(raw);

    return MAA_RES_NS::PipelineResMgr::check_all_validity(all);
}

MAA_TASK_NS_END
