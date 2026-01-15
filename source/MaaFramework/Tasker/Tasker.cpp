#include "RuntimeCache.h"

#include <ranges>

#include "Controller/ControllerAgent.h"
#include "Global/PluginMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Uuid.h"
#include "Resource/ResourceMgr.h"
#include "Task/ActionTask.h"
#include "Task/EmptyTask.h"
#include "Task/PipelineTask.h"
#include "Task/RecognitionTask.h"

MAA_NS_BEGIN

Tasker::Tasker()
{
    LogFunc;

    auto& plugin_mgr = MAA_GLOBAL_NS::PluginMgr::get_instance();
    LogInfo << VAR(plugin_mgr.get_names());

    for (const auto& sink : plugin_mgr.get_tasker_sinks()) {
        add_sink(sink, this);
    }
    for (const auto& sink : plugin_mgr.get_ctx_sinks()) {
        add_context_sink(sink, this);
    }

    task_runner_ = std::make_unique<AsyncRunner<TaskPtr>>(std::bind(&Tasker::run_task, this, std::placeholders::_1, std::placeholders::_2));
}

Tasker::~Tasker()
{
    LogFunc;

    if (task_runner_) {
        task_runner_->wait_all();
    }
}

bool Tasker::bind_resource(MaaResource* resource)
{
    auto* derived = dynamic_cast<MAA_RES_NS::ResourceMgr*>(resource);

    LogInfo << VAR_VOIDP(this) << VAR_VOIDP(resource) << VAR_VOIDP(derived) << VAR_VOIDP(resource_);

    if (resource && !derived) {
        LogError << "Invalid resource";
        return false;
    }

    resource_ = derived;
    return true;
}

bool Tasker::bind_controller(MaaController* controller)
{
    auto* derived = dynamic_cast<MAA_CTRL_NS::ControllerAgent*>(controller);

    LogInfo << VAR_VOIDP(this) << VAR_VOIDP(controller) << VAR_VOIDP(derived) << VAR_VOIDP(controller_);

    if (controller && !derived) {
        LogError << "Invalid controller";
        return false;
    }

    controller_ = derived;
    return true;
}

bool Tasker::inited() const
{
    return resource_ && controller_ && resource_->valid() && controller_->connected();
}

bool Tasker::set_option(MaaTaskerOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    std::ignore = key;
    std::ignore = value;
    std::ignore = val_size;

    return false;
}

MaaTaskId Tasker::post_task(const std::string& entry, const json::value& pipeline_override)
{
    LogInfo << VAR(entry) << VAR(pipeline_override);

    if (!check_stop()) {
        return MaaInvalidId;
    }

    auto task_ptr = std::make_shared<MAA_TASK_NS::PipelineTask>(entry, this);
    return post_task(std::move(task_ptr), pipeline_override);
}

MaaTaskId Tasker::post_recognition(const std::string& reco_type, const json::value& reco_param, const cv::Mat& image)
{
    LogInfo << VAR(reco_type) << VAR(reco_param) << VAR(image);

    if (!check_stop()) {
        return MaaInvalidId;
    }

    std::string entry = std::format("recognition/{}/{}", reco_type, make_uuid());

    json::value pipeline_override;
    pipeline_override[entry]["recognition"] = { { "type", reco_type }, { "param", reco_param } };

    auto task_ptr = std::make_shared<MAA_TASK_NS::RecognitionTask>(image, entry, this);
    return post_task(std::move(task_ptr), pipeline_override);
}

MaaTaskId Tasker::post_action(
    const std::string& action_type,
    const json::value& action_param,
    const cv::Rect& box,
    const std::string& reco_detail)
{
    LogInfo << VAR(action_type) << VAR(action_param) << VAR(box) << VAR(reco_detail);

    if (!check_stop()) {
        return MaaInvalidId;
    }

    std::string entry = std::format("action/{}/{}", action_type, make_uuid());

    json::value pipeline_override;
    pipeline_override[entry]["action"] = { { "type", action_type }, { "param", action_param } };

    auto task_ptr = std::make_shared<MAA_TASK_NS::ActionTask>(box, reco_detail, entry, this);
    return post_task(std::move(task_ptr), pipeline_override);
}

MaaStatus Tasker::status(MaaTaskId task_id) const
{
    if (!task_runner_) {
        LogError << "task_runner is nullptr";
        return MaaStatus_Invalid;
    }
    RunnerId runner_id = task_id_to_runner_id(task_id);
    return static_cast<MaaStatus>(task_runner_->status(runner_id));
}

MaaStatus Tasker::wait(MaaTaskId task_id) const
{
    if (!task_runner_) {
        LogError << "task_runner is nullptr";
        return MaaStatus_Invalid;
    }
    RunnerId runner_id = task_id_to_runner_id(task_id);
    task_runner_->wait(runner_id);
    return status(task_id);
}

bool Tasker::running() const
{
    return task_runner_ && task_runner_->running();
}

MaaTaskId Tasker::post_stop()
{
    LogFunc;

    need_to_stop_ = true;

    if (task_runner_ && task_runner_->running()) {
        task_runner_->clear();
    }
    if (running_task_) {
        running_task_->post_stop();
    }
    if (resource_) {
        resource_->post_stop();
    }
    if (controller_) {
        controller_->post_stop();
    }

    static const std::string kStopEntry = "MaaTaskerPostStop";
    auto task_ptr = std::make_shared<MAA_TASK_NS::EmptyTask>(kStopEntry, this);
    return post_task(std::move(task_ptr), json::object {});
}

bool Tasker::stopping() const
{
    return need_to_stop_ && running();
}

MAA_RES_NS::ResourceMgr* Tasker::resource() const
{
    return resource_;
}

MAA_CTRL_NS::ControllerAgent* Tasker::controller() const
{
    return controller_;
}

void Tasker::clear_cache()
{
    LogTrace;

    {
        std::unique_lock lock(task_id_mapping_mutex_);
        task_id_mapping_.clear();
    }

    runtime_cache().clear();
}

std::optional<MAA_TASK_NS::TaskDetail> Tasker::get_task_detail(MaaTaskId task_id) const
{
    return runtime_cache().get_task_detail(task_id);
}

std::optional<MAA_TASK_NS::NodeDetail> Tasker::get_node_detail(MaaNodeId node_id) const
{
    return runtime_cache().get_node_detail(node_id);
}

std::optional<MAA_TASK_NS::RecoResult> Tasker::get_reco_result(MaaRecoId reco_id) const
{
    return runtime_cache().get_reco_result(reco_id);
}

std::optional<MAA_TASK_NS::ActionResult> Tasker::get_action_result(MaaActId action_id) const
{
    return runtime_cache().get_action_result(action_id);
}

std::optional<MaaNodeId> Tasker::get_latest_node(const std::string& node_name) const
{
    return runtime_cache().get_latest_node(node_name);
}

RuntimeCache& Tasker::runtime_cache()
{
    return runtime_cache_;
}

const RuntimeCache& Tasker::runtime_cache() const
{
    return runtime_cache_;
}

MaaSinkId Tasker::add_sink(MaaEventCallback callback, void* trans_arg)
{
    return notifier_.add_sink(callback, trans_arg);
}

void Tasker::remove_sink(MaaSinkId sink_id)
{
    notifier_.remove_sink(sink_id);
}

void Tasker::clear_sinks()
{
    notifier_.clear_sinks();
}

MaaSinkId Tasker::add_context_sink(MaaEventCallback callback, void* trans_arg)
{
    return context_notifier_.add_sink(callback, trans_arg);
}

void Tasker::remove_context_sink(MaaSinkId sink_id)
{
    context_notifier_.remove_sink(sink_id);
}

void Tasker::clear_context_sinks()
{
    context_notifier_.clear_sinks();
}

void Tasker::context_notify(MaaContext* context, std::string_view msg, const json::value& details)
{
    context_notifier_.notify(context, msg, details);
}

MaaTaskId Tasker::post_task(TaskPtr task_ptr, const json::value& pipeline_override)
{
#ifndef MAA_DEBUG
    if (!inited()) {
        LogError << "Tasker not inited";
        return MaaInvalidId;
    }
#endif

    MaaTaskId task_id = task_ptr->task_id();
    bool ov = task_ptr->override_pipeline(pipeline_override);
    if (!ov) {
        LogError << "failed to override_pipeline" << VAR(task_id) << VAR(task_ptr->entry()) << VAR(pipeline_override);
        return MaaInvalidId;
    }

    runtime_cache_.set_task_detail(
        task_id,
        MAA_TASK_NS::TaskDetail { .task_id = task_id, .entry = task_ptr->entry(), .status = MaaStatus_Pending });

    std::unique_lock lock(task_id_mapping_mutex_);

    RunnerId runner_id = task_runner_->post(task_ptr);
    task_id_mapping_.emplace(task_id, runner_id);

    return task_id;
}

bool Tasker::run_task(RunnerId runner_id, TaskPtr task_ptr)
{
    if (!need_to_stop_) {
        MAA_LOG_NS::Logger::get_instance().flush();
    }

    LogFunc << VAR(runner_id) << VAR(task_ptr);

    if (!task_ptr) {
        LogError << "task_ptr is nullptr";
        return false;
    }

    running_task_ = task_ptr;
    OnScopeLeave([&] { running_task_ = nullptr; });

    if (need_to_stop_) {
        running_task_->post_stop();
    }

    MaaTaskId task_id = task_ptr->task_id();
    const std::string& entry = task_ptr->entry();
    const json::value cb_detail = {
        { "task_id", task_id },
        { "entry", entry },
        { "hash", resource_ ? resource_->get_hash() : std::string() },
        { "uuid", controller_ ? controller_->get_uuid() : std::string() },
    };

    LogInfo << "task start:" << VAR(cb_detail);

    {
        // value_or 的默认值用于在 post 之后调用方手动 clear cache 了的情况
        auto task_detail = runtime_cache_.get_task_detail(task_id).value_or(MAA_TASK_NS::TaskDetail { .task_id = task_id, .entry = entry });
        task_detail.status = MaaStatus_Running;
        runtime_cache_.set_task_detail(task_id, std::move(task_detail));
    }
    notifier_.notify(this, MaaMsg_Tasker_Task_Starting, cb_detail);

    bool ret = task_ptr->run();

    LogInfo << "task end:" << VAR(cb_detail) << VAR(ret);
    {
        // value_or 的默认值用于 run 到一半调用方手动 clear cache 了的情况
        auto task_detail = runtime_cache_.get_task_detail(task_id).value_or(MAA_TASK_NS::TaskDetail { .task_id = task_id, .entry = entry });
        task_detail.status = ret ? MaaStatus_Succeeded : MaaStatus_Failed;
        runtime_cache_.set_task_detail(task_id, std::move(task_detail));
    }
    notifier_.notify(this, ret ? MaaMsg_Tasker_Task_Succeeded : MaaMsg_Tasker_Task_Failed, cb_detail);

    running_task_ = nullptr;

    return ret;
}

bool Tasker::check_stop()
{
    if (!need_to_stop_) {
        return true;
    }

    if (running()) {
        LogError << "stopping, ignore new post";
        return false;
    }

    need_to_stop_ = false;
    return true;
}

Tasker::RunnerId Tasker::task_id_to_runner_id(MaaTaskId task_id) const
{
    std::shared_lock lock(task_id_mapping_mutex_);

    auto iter = task_id_mapping_.find(task_id);
    if (iter == task_id_mapping_.end()) {
        LogError << "runner id not found" << VAR(task_id);
        return MaaInvalidId;
    }
    return iter->second;
}

MAA_NS_END
