#include "Context.h"

#include <meojson/json.hpp>

#include "ActionTask.h"
#include "MaaUtils/Logger.h"
#include "PipelineTask.h"
#include "RecognitionTask.h"
#include "Resource/PipelineChecker.h"
#include "Resource/PipelineDumper.h"
#include "Resource/PipelineParser.h"
#include "Tasker/Tasker.h"

MAA_TASK_NS_BEGIN

Context::Context(MaaTaskId id, Tasker* tasker, PrivateArg)
    : task_id_(id)
    , tasker_(tasker)
    , task_state_(std::make_shared<TaskState>())
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

std::shared_ptr<Context> Context::make_clone() const
{
    return std::make_shared<Context>(*this);
}

Context::Context(const Context& other)
    : std::enable_shared_from_this<Context>(other)
    , task_id_(other.task_id_)
    , tasker_(other.tasker_)
    , pipeline_override_(other.pipeline_override_)
    , image_override_(other.image_override_)
    , task_state_(other.task_state_)
// don't copy clone_holder_
{
    LogDebug << VAR(other.getptr());
}

MaaTaskId Context::run_task(const std::string& entry, const json::value& pipeline_override)
{
    LogTrace << VAR(getptr()) << VAR(entry) << VAR(pipeline_override);

    if (!tasker_) {
        LogError << "tasker is null";
        return MaaInvalidId;
    }

    PipelineTask subtask(entry, tasker_, make_clone());
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

MaaRecoId Context::run_recognition(const std::string& entry, const json::value& pipeline_override, const cv::Mat& image)
{
    LogTrace << VAR(getptr()) << VAR(entry) << VAR(pipeline_override);

    RecognitionTask subtask(image, entry, tasker_, make_clone());
    bool ov = subtask.override_pipeline(pipeline_override);
    if (!ov) {
        LogError << "failed to override_pipeline" << VAR(entry) << VAR(pipeline_override);
        return MaaInvalidId;
    }
    return subtask.run_impl();
}

MaaActId
    Context::run_action(const std::string& entry, const json::value& pipeline_override, const cv::Rect& box, const std::string& reco_detail)
{
    LogTrace << VAR(getptr()) << VAR(entry) << VAR(pipeline_override) << VAR(box) << VAR(reco_detail);

    ActionTask subtask(box, reco_detail, entry, tasker_, make_clone());
    bool ov = subtask.override_pipeline(pipeline_override);
    if (!ov) {
        LogError << "failed to override_pipeline" << VAR(entry) << VAR(pipeline_override);
        return MaaInvalidId;
    }
    return subtask.run_impl();
}

bool Context::override_pipeline(const json::value& pipeline_override)
{
    LogTrace << VAR(getptr()) << VAR(pipeline_override);

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

    bool ret = false;
    if (pipeline_override.is_object()) {
        ret = override_pipeline_once(pipeline_override.as_object(), default_mgr);
    }
    else if (pipeline_override.is_array()) {
        ret = true;
        for (const auto& val : pipeline_override.as_array()) {
            if (!val.is_object()) {
                LogError << "input is not json array of object" << VAR(pipeline_override);
                return false;
            }
            ret &= override_pipeline_once(val.as_object(), default_mgr);
        }
    }
    else {
        LogError << "input is invalid" << VAR(pipeline_override);
        return false;
    }

    return ret && check_pipeline();
}

bool Context::override_pipeline_once(const json::object& pipeline_override, const MAA_RES_NS::DefaultPipelineMgr& default_mgr)
{
    // LogTrace << VAR(getptr()) << VAR(pipeline_override);

    for (const auto& [key, value] : pipeline_override) {
        PipelineData result;
        auto default_result = get_pipeline_data(key).value_or(default_mgr.get_pipeline());
        bool ret = MAA_RES_NS::PipelineParser::parse_node(key, value, result, default_result, default_mgr);
        if (!ret) {
            LogError << "parse_task failed" << VAR(key) << VAR(value);
            return false;
        }

        pipeline_override_.insert_or_assign(key, std::move(result));
    }

    return true;
}

bool Context::override_next(const std::string& node_name, const std::vector<std::string>& next)
{
    LogTrace << VAR(getptr()) << VAR(node_name) << VAR(next);

    auto data_opt = get_pipeline_data(node_name);
    if (!data_opt) {
        LogError << "get_pipeline_data failed, task not exist" << VAR(node_name);
        return false;
    }

    if (!MAA_RES_NS::PipelineParser::parse_next(next, data_opt->next)) {
        LogError << "failed to parse_next" << VAR(next);
        return false;
    }

    pipeline_override_.insert_or_assign(node_name, std::move(*data_opt));

    return check_pipeline();
}

bool Context::override_image(const std::string& image_name, const cv::Mat& image)
{
    LogInfo << VAR(getptr()) << VAR(image_name) << VAR(image);

    image_override_.insert_or_assign(image_name, image);
    return true;
}

Context* Context::clone() const
{
    auto& ref = clone_holder_.emplace_back(make_clone());
    LogDebug << VAR(getptr()) << VAR(ref);

    return ref.get();
}

std::optional<json::object> Context::get_node_data(const std::string& node_name) const
{
    auto pp_opt = get_pipeline_data(node_name);
    if (!pp_opt) {
        return std::nullopt;
    }

    return MAA_RES_NS::PipelineDumper::dump(*pp_opt);
}

MaaTaskId Context::task_id() const
{
    return task_id_;
}

Tasker* Context::tasker() const
{
    return tasker_;
}

std::optional<PipelineData> Context::get_pipeline_data(const std::string& node_name) const
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

    const auto& raw_pp_map = resource->pipeline_res().get_pipeline_data_map();
    auto raw_it = raw_pp_map.find(node_name);
    if (raw_it != raw_pp_map.end()) {
        return raw_it->second;
    }

    LogWarn << "task not found" << VAR(node_name);
    return std::nullopt;
}

std::optional<PipelineData> Context::get_pipeline_data(const MAA_RES_NS::NodeAttr& node_attr) const
{
    std::string node_name = node_attr.name;
    if (node_attr.anchor) {
        auto anchor_node = get_anchor(node_attr.name);
        if (!anchor_node) {
            LogDebug << "anchor not set" << VAR(node_attr.name);
            return std::nullopt;
        }
        node_name = *anchor_node;
    }
    return get_pipeline_data(node_name);
}

std::vector<cv::Mat> Context::get_images(const std::vector<std::string>& names)
{
    if (!tasker_) {
        LogError << "tasker is null";
        return {};
    }
    auto* resource = tasker_->resource();
    if (!resource) {
        LogError << "resource not bound";
        return {};
    }

    std::vector<cv::Mat> results;

    for (const std::string& name : names) {
        auto it = image_override_.find(name);
        if (it != image_override_.end()) {
            LogTrace << "image override" << VAR(name);
            results.emplace_back(it->second);
            continue;
        }

        auto imgs = resource->template_res().get_image(name);
        results.insert(results.end(), std::make_move_iterator(imgs.begin()), std::make_move_iterator(imgs.end()));
    }

    return results;
}

bool& Context::need_to_stop()
{
    return need_to_stop_;
}

size_t Context::get_hit_count(const std::string& node_name) const
{
    auto it = task_state_->hit_count.find(node_name);
    if (it != task_state_->hit_count.end()) {
        return it->second;
    }
    return 0;
}

void Context::increment_hit_count(const std::string& node_name)
{
    task_state_->hit_count[node_name]++;
}

void Context::clear_hit_count(const std::string& node_name)
{
    task_state_->hit_count.erase(node_name);
}

void Context::set_anchor(const std::string& anchor_name, const std::string& node_name)
{
    LogDebug << VAR(anchor_name) << VAR(node_name);
    task_state_->anchors[anchor_name] = node_name;
}

std::optional<std::string> Context::get_anchor(const std::string& anchor_name) const
{
    auto it = task_state_->anchors.find(anchor_name);
    if (it != task_state_->anchors.end()) {
        return it->second;
    }
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
        LogError << "resource not bound";
        return false;
    }

    auto raw = resource->pipeline_res().get_pipeline_data_map();
    auto all = pipeline_override_;
    all.merge(raw);

    return MAA_RES_NS::PipelineChecker::check_all_validity(all);
}

MAA_TASK_NS_END
