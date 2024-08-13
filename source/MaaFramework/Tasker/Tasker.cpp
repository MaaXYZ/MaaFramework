#include "RuntimeCache.h"

#include "Controller/ControllerAgent.h"
#include "MaaFramework/MaaMsg.h"
#include "Resource/ResourceMgr.h"
#include "Task/PipelineTask.h"
#include "Task/Recognizer.h"
#include "Utils/Logger.h"

MAA_NS_BEGIN

Tasker::Tasker(MaaNotificationCallback callback, MaaTransparentArg callback_arg)
    : notifier(callback, callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    task_runner_ =
        std::make_unique<AsyncRunner<TaskPtr>>(std::bind(&Tasker::run_task, this, std::placeholders::_1, std::placeholders::_2));
}

Tasker::~Tasker()
{
    LogFunc;

    if (task_runner_) {
        task_runner_->release();
    }
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

MaaTaskId Tasker::post_pipeline(std::string entry, std::string_view param)
{
    auto task = make_task(entry, param);
    if (!task) {
        LogError << "failed to make task" << VAR(entry) << VAR(param);
        return MaaInvalidId;
    }
    task->set_type(MAA_TASK_NS::PipelineTask::RunType::Pipeline);

    return post_task(task);
}

MaaTaskId Tasker::post_recognition(std::string entry, std::string_view param)
{
    auto task = make_task(entry, param);
    if (!task) {
        LogError << "failed to make task" << VAR(entry) << VAR(param);
        return MaaInvalidId;
    }
    task->set_type(MAA_TASK_NS::PipelineTask::RunType::Recognition);

    return post_task(task);
}

MaaTaskId Tasker::post_action(std::string entry, std::string_view param)
{
    auto task = make_task(entry, param);
    if (!task) {
        LogError << "failed to make task" << VAR(entry) << VAR(param);
        return MaaInvalidId;
    }
    task->set_type(MAA_TASK_NS::PipelineTask::RunType::Action);

    return post_task(task);
}

bool Tasker::set_param(MaaTaskId task_id, std::string_view param)
{
    LogInfo << VAR(task_id) << VAR(param);

    auto param_opt = json::parse(param);
    if (!param_opt) {
        LogError << "Invalid param:" << param;
        return false;
    }

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

    return task_ptr->set_param(*param_opt);
}

MaaStatus Tasker::status(MaaTaskId task_id) const
{
    if (!task_runner_) {
        LogError << "task_runner is nullptr";
        return MaaStatus_Invalid;
    }
    return task_runner_->status(task_id);
}

MaaStatus Tasker::wait(MaaTaskId task_id) const
{
    if (!task_runner_) {
        LogError << "task_runner is nullptr";
        return MaaStatus_Invalid;
    }
    task_runner_->wait(task_id);
    return task_runner_->status(task_id);
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

MaaResource* Tasker::resource()
{
    return resource_;
}

MaaController* Tasker::controller()
{
    return controller_;
}

RuntimeCache& Tasker::runtime_cache()
{
    return runtime_cache_;
}

const RuntimeCache& Tasker::runtime_cache() const
{
    return runtime_cache_;
}

Tasker::TaskPtr Tasker::make_task(std::string entry, std::string_view param)
{
    LogInfo << VAR(entry) << VAR(param);

#ifndef MAA_DEBUG
    if (!inited()) {
        LogError << "Tasker not inited";
        return nullptr;
    }
#endif

    if (!check_stop()) {
        return nullptr;
    }

    TaskPtr task_ptr = std::make_shared<TaskNS::PipelineTask>(std::move(entry), this);

    auto param_opt = json::parse(param);
    if (!param_opt) {
        LogError << "Invalid param:" << param;
        return nullptr;
    }

    bool param_ret = task_ptr->set_param(*param_opt);
    if (!param_ret) {
        LogError << "Set task param failed:" << param;
        return nullptr;
    }

    return task_ptr;
}

Tasker::TaskId Tasker::post_task(const TaskPtr& task_ptr)
{
    auto id = task_runner_->post(task_ptr);

    {
        std::unique_lock lock(task_cache_mutex_);
        task_cache_.emplace(id, task_ptr);
    }

    LogTrace << VAR(id);
    return id;
}

bool Tasker::run_task(TaskId id, TaskPtr task_ptr)
{
    LogFunc << VAR(id) << VAR(task_ptr);

    if (!task_ptr) {
        LogError << "task_ptr is nullptr";
        return false;
    }

    const json::value details = {
        { "id", id },
        { "entry", task_ptr->entry() },
        { "name", task_ptr->entry() },
        { "hash", resource_ ? resource_->get_hash() : std::string() },
        { "uuid", controller_ ? controller_->get_uuid() : std::string() },
    };

    notifier.notify(MaaMsg_Tasker_Task_Started, details);

    LogInfo << "task start:" << VAR(details);

    task_ptr->set_taskid(id);
    bool ret = task_ptr->run();

    LogInfo << "task end:" << VAR(details) << VAR(ret);

    notifier.notify(ret ? MaaMsg_Tasker_Task_Completed : MaaMsg_Tasker_Task_Failed, details);

    {
        std::unique_lock lock(task_cache_mutex_);
        task_cache_.erase(id);
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

MAA_NS_END
