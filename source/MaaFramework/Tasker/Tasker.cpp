#include "RuntimeCache.h"

#include "Controller/ControllerAgent.h"
#include "MaaFramework/MaaMsg.h"
#include "Resource/ResourceMgr.h"
#include "Task/ActionTask.h"
#include "Task/PipelineTask.h"
#include "Task/RecognitionTask.h"
#include "Utils/Logger.h"

MAA_NS_BEGIN

Tasker::Tasker(MaaNotificationCallback callback, void* callback_arg)
    : notifier(callback, callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    task_runner_ = std::make_unique<AsyncRunner<TaskPtr>>(std::bind(&Tasker::run_task, this, std::placeholders::_1, std::placeholders::_2));
}

Tasker::~Tasker()
{
    LogFunc;
}

bool Tasker::bind_resource(MaaResource* resource)
{
    LogInfo << VAR_VOIDP(this) << VAR_VOIDP(resource);

    resource_ = dynamic_cast<MAA_RES_NS::ResourceMgr*>(resource);

    if (!resource) {
        LogError << "Invalid resource";
        return false;
    }

    return true;
}

bool Tasker::bind_controller(MaaController* controller)
{
    LogInfo << VAR_VOIDP(this) << VAR_VOIDP(controller);

    controller_ = dynamic_cast<MAA_CTRL_NS::ControllerAgent*>(controller);

    if (!controller) {
        LogError << "Invalid controller";
        return false;
    }
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

MaaTaskId Tasker::post_pipeline(const std::string& entry, const json::value& pipeline_override)
{
    LogInfo << VAR(entry) << VAR(pipeline_override);

    auto task_ptr = std::make_shared<MAA_TASK_NS::PipelineTask>(entry, this, MAA_TASK_NS::TaskBase::PipelineDataMap {});
    return post_task(std::move(task_ptr), pipeline_override);
}

MaaTaskId Tasker::post_recognition(const std::string& entry, const json::value& pipeline_override)
{
    LogInfo << VAR(entry) << VAR(pipeline_override);

    auto task_ptr = std::make_shared<MAA_TASK_NS::RecognitionTask>(entry, this, MAA_TASK_NS::TaskBase::PipelineDataMap {});
    return post_task(std::move(task_ptr), pipeline_override);
}

MaaTaskId Tasker::post_action(const std::string& entry, const json::value& pipeline_override)
{
    LogInfo << VAR(entry) << VAR(pipeline_override);

    auto task_ptr = std::make_shared<MAA_TASK_NS::ActionTask>(entry, this, MAA_TASK_NS::TaskBase::PipelineDataMap {});
    return post_task(std::move(task_ptr), pipeline_override);
}

bool Tasker::override_pipeline(MaaTaskId task_id, const json::value& pipeline_override)
{
    LogInfo << VAR(task_id) << VAR(pipeline_override);

    TaskPtr task_ptr;
    {
        std::unique_lock lock(task_cache_mutex_);

        auto it = task_cache_.find(task_id);
        if (it == task_cache_.end()) {
            LogError << "task_id not found:" << task_id;
            return false;
        }

        task_ptr = it->second;
    }

    if (!task_ptr) {
        LogError << "task_ptr is nullptr";
        return false;
    }

    return task_ptr->override_pipeline(pipeline_override);
}

MaaStatus Tasker::status(MaaTaskId task_id) const
{
    if (!task_runner_) {
        LogError << "task_runner is nullptr";
        return MaaStatus_Invalid;
    }
    RunnerId runner_id = task_id_to_runner_id(task_id);
    return task_runner_->status(runner_id);
}

MaaStatus Tasker::wait(MaaTaskId task_id) const
{
    if (!task_runner_) {
        LogError << "task_runner is nullptr";
        return MaaStatus_Invalid;
    }
    RunnerId runner_id = task_id_to_runner_id(task_id);
    task_runner_->wait(runner_id);
    return task_runner_->status(runner_id);
}

MaaBool Tasker::running() const
{
    return resource_ && resource_->running() && controller_ && controller_->running() && task_runner_ && task_runner_->running();
}

void Tasker::post_stop()
{
    LogFunc;

    need_to_stop_ = true;

    if (task_runner_ && task_runner_->running()) {
        task_runner_->clear();
    }

    decltype(task_cache_) tmp_cache;
    {
        std::unique_lock lock(task_cache_mutex_);
        tmp_cache = std::move(task_cache_);
        task_cache_.clear();
    }

    for (auto& task_ptr : tmp_cache | std::views::values) {
        if (!task_ptr) {
            continue;
        }
        task_ptr->post_stop();
    }

    if (resource_) {
        resource_->post_stop();
    }
    if (controller_) {
        controller_->post_stop();
    }
}

MAA_RES_NS::ResourceMgr* Tasker::resource()
{
    return resource_;
}

MAA_CTRL_NS::ControllerAgent* Tasker::controller()
{
    return controller_;
}

void Tasker::clear_cache()
{
    LogTrace;

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

std::optional<MaaNodeId> Tasker::get_latest_node(const std::string& task_name) const
{
    return runtime_cache().get_latest_node(task_name);
}

RuntimeCache& Tasker::runtime_cache()
{
    return runtime_cache_;
}

const RuntimeCache& Tasker::runtime_cache() const
{
    return runtime_cache_;
}

void Tasker::notify(std::string_view msg, json::value detail)
{
    notifier.notify(msg, detail);
}

MaaTaskId Tasker::post_task(TaskPtr task_ptr, const json::value& pipeline_override)
{
#ifndef MAA_DEBUG
    if (!inited()) {
        LogError << "Tasker not inited";
        return MaaInvalidId;
    }
#endif

    if (!check_stop()) {
        return MaaInvalidId;
    }

    task_ptr->override_pipeline(pipeline_override);
    MaaTaskId task_id = task_ptr->task_id();

    {
        std::unique_lock lock(task_cache_mutex_);

        RunnerId runner_id = task_runner_->post(task_ptr);

        task_cache_.emplace(task_id, task_ptr);
        task_id_mapping_.emplace(task_id, runner_id);
        runner_id_mapping_.emplace(runner_id, task_id);
    }

    return task_id;
}

bool Tasker::run_task(RunnerId runner_id, TaskPtr task_ptr)
{
    LogFunc << VAR(runner_id) << VAR(task_ptr);

    if (!task_ptr) {
        LogError << "task_ptr is nullptr";
        return false;
    }
    MaaTaskId task_id = runner_id_to_task_id(runner_id);

    const json::value details = {
        { "id", task_id },
        { "entry", task_ptr->entry() },
        { "name", task_ptr->entry() },
        { "hash", resource_ ? resource_->get_hash() : std::string() },
        { "uuid", controller_ ? controller_->get_uuid() : std::string() },
    };

    notifier.notify(MaaMsg_Tasker_Task_Started, details);

    LogInfo << "task start:" << VAR(details);

    bool ret = task_ptr->run();

    LogInfo << "task end:" << VAR(details) << VAR(ret);

    notifier.notify(ret ? MaaMsg_Tasker_Task_Completed : MaaMsg_Tasker_Task_Failed, details);

    {
        std::unique_lock lock(task_cache_mutex_);
        task_cache_.erase(task_id);
        task_id_mapping_.erase(task_id);
        runner_id_mapping_.erase(runner_id);
    }

    MAA_LOG_NS::Logger::get_instance().flush();

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
    std::unique_lock lock(task_cache_mutex_);

    auto iter = task_id_mapping_.find(task_id);
    if (iter == task_id_mapping_.end()) {
        LogError << "runner id not found" << VAR(task_id);
        return {};
    }
    return iter->second;
}

MaaTaskId Tasker::runner_id_to_task_id(RunnerId runner_id) const
{
    std::unique_lock lock(task_cache_mutex_);

    auto iter = runner_id_mapping_.find(runner_id);
    if (iter == runner_id_mapping_.end()) {
        LogError << "task id not found" << VAR(runner_id);
        return {};
    }
    return iter->second;
}

MAA_NS_END
