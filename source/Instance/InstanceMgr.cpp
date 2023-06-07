#include "InstanceMgr.h"

#include "Task/PipelineTask.h"
#include "Utils/Logger.hpp"
#include "MaaParam.h"

MAA_NS_BEGIN

InstanceMgr::InstanceMgr(MaaInstanceCallback callback, void* callback_arg) : notifier(callback, callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    task_runner_ = std::make_unique<AsyncRunner<TaskPtr>>(
        std::bind(&InstanceMgr::run_task, this, std::placeholders::_1, std::placeholders::_2));
}

InstanceMgr::~InstanceMgr()
{
    LogFunc;

    if (task_runner_) {
        task_runner_->release();
    }

    notifier.release();
}

bool InstanceMgr::bind_resource(MaaResourceAPI* resource)
{
    LogFunc << VAR_VOIDP(resource);

    if (!resource) {
        LogError << "Invalid resource";
        return false;
    }

    if (!resource->loaded()) {
        LogError << "Resource not loaded";
        return false;
    }

    resource_ = resource;
    return true;
}

bool InstanceMgr::bind_controller(MaaControllerAPI* controller)
{
    LogFunc << VAR_VOIDP(controller);

    if (!controller) {
        LogError << "Invalid controller";
        return false;
    }

    if (!controller->connected()) {
        LogError << "Controller not connected";
        return false;
    }

    controller_ = controller;
    return true;
}

bool InstanceMgr::inited() const
{
    return resource_ && controller_ && resource_->loaded() && controller_->connected();
}

bool InstanceMgr::set_option(std::string_view key, std::string_view value)
{
    return false;
}

MaaTaskId InstanceMgr::post_task(std::string_view type, std::string_view param)
{
    LogInfo << VAR(type) << VAR(param);

    TaskPtr task_ptr = nullptr;
    if (type == MaaTaskType_Pipeline) {
        task_ptr = std::make_shared<TaskNS::PipelineTask>();
    }
    else {
        LogError << "Unknown task type:" << type;
        return MaaInvalidId;
    }
    
    auto param_opt = json::parse(param);
    if (!param_opt) {
        LogError << "Invalid param:" << param;
        return MaaInvalidId;
    }

    bool param_ret = task_ptr->set_param(*param_opt);
    if (!param_ret) {
        LogError << "Set task param failed:" << param;
        return MaaInvalidId;
    }

    auto id = task_runner_->post(task_ptr);
    LogTrace << task_ptr->type() << VAR(id);

    std::unique_lock<std::mutex> lock(task_mutex_);
    task_map_.emplace(id, task_ptr);
    lock.unlock();

    return id;
}

bool InstanceMgr::set_task_param(MaaTaskId task_id, std::string_view param)
{
    LogInfo << VAR(task_id) << VAR(param);

    std::unique_lock<std::mutex> lock(task_mutex_);
    auto iter = task_map_.find(task_id);
    if (iter == task_map_.end()) {
        LogError << "Invalid task id:" << task_id;
        return false;
    }

    auto task_ptr = iter->second;
    lock.unlock();

    if (!task_ptr) {
        LogError << "Invalid task ptr:" << task_id;
        return false;
    }

    auto param_opt = json::parse(param);
    if (!param_opt) {
        LogError << "Invalid param:" << param;
        return false;
    }

    bool ret = task_ptr->set_param(*param_opt);

    LogTrace << task_ptr->type() << VAR(ret);
    return ret;
}

MaaStatus InstanceMgr::status(MaaTaskId task_id) const
{
    return task_runner_->status(task_id);
}

MaaBool InstanceMgr::all_finished() const
{
    return !task_runner_->running();
}

void InstanceMgr::stop()
{
    LogFunc;

    std::unique_lock<std::mutex> lock(task_mutex_);
    task_map_.clear();
    lock.unlock();

    task_runner_->clear();
}

std::string InstanceMgr::get_resource_hash() const
{
    return resource_ ? resource_->get_hash() : std::string();
}

std::string InstanceMgr::get_controller_uuid() const
{
    return controller_ ? controller_->get_uuid() : std::string();
}

bool InstanceMgr::run_task(typename AsyncRunner<TaskPtr>::Id id, TaskPtr task_ptr)
{
    LogFunc << VAR(id) << VAR(task_ptr);

    const json::value details = {
        { "id", id },
        { "task", std::string(task_ptr->type()) },
        { "uuid", get_controller_uuid() },
        { "hash", get_resource_hash() },
    };

    notifier.notify(MaaMsg::TaskStarted, details);

    bool ret = task_ptr->run();

    notifier.notify(ret ? MaaMsg::TaskCompleted : MaaMsg::TaskFailed, details);

    std::unique_lock<std::mutex> lock(task_mutex_);
    task_map_.erase(id);

    return ret;
}

MAA_NS_END
