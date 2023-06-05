#include "InstanceMgr.h"

#include "Task/PipelineTask.h"
#include "Utils/Logger.hpp"

MAA_NS_BEGIN

InstanceMgr::InstanceMgr(MaaInstanceCallback callback, void* callback_arg)
    : callback_(callback), callback_arg_(callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    task_runner_ = std::make_unique<AsyncRunner<TaskPtr>>(
        std::bind(&InstanceMgr::run_task, this, std::placeholders::_1, std::placeholders::_2));
    notify_runner_ = std::make_unique<AsyncRunner<NotifyData>>(
        std::bind(&InstanceMgr::notify, this, std::placeholders::_1, std::placeholders::_2));
}

InstanceMgr::~InstanceMgr()
{
    LogFunc;
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

MaaTaskId InstanceMgr::append_task(std::string_view type, std::string_view param)
{
    LogFunc << VAR(type) << VAR(param);

    auto param_opt = json::parse(param);
    if (!param_opt) {
        LogError << "Invalid param:" << param;
        return MaaInvalidId;
    }

    // TODO: check task type

    TaskPtr task_ptr = std::make_shared<TaskNS::PipelineTask>();
    bool ret = task_ptr->set_param(*param_opt);
    if (!ret) {
        LogError << "Set task param failed:" << param;
        return MaaInvalidId;
    }

    auto id = task_runner_->append(task_ptr);
    LogInfo << "Append task:" << task_ptr->type() << VAR(id);
    task_map_.emplace(id, task_ptr);

    return id;
}

bool InstanceMgr::set_task_param(MaaTaskId task_id, std::string_view param)
{
    LogFunc << VAR(task_id) << VAR(param);

    auto iter = task_map_.find(task_id);
    if (iter == task_map_.end()) {
        LogError << "Invalid task id:" << task_id;
        return false;
    }

    auto param_opt = json::parse(param);
    if (!param_opt) {
        LogError << "Invalid param:" << param;
        return false;
    }

    auto& task_ptr = iter->second;
    if (!task_ptr) {
        LogError << "Invalid task ptr:" << task_id;
        return false;
    }

    bool ret = task_ptr->set_param(*param_opt);

    LogInfo << "Set task param:" << task_ptr->type() << VAR(ret);
    return ret;
}

std::vector<MaaTaskId> InstanceMgr::get_task_list() const
{
    std::vector<MaaTaskId> result;
    ranges::transform(task_map_ | views::keys, std::back_inserter(result), [](auto id) { return id; });
    return result;
}

void InstanceMgr::stop()
{
    LogFunc;

    task_map_.clear();
    task_runner_->clear();
}

bool InstanceMgr::running() const
{
    return task_runner_->running();
}

std::string InstanceMgr::get_resource_hash() const
{
    return resource_ ? resource_->get_hash() : std::string();
}

std::string InstanceMgr::get_controller_uuid() const
{
    return controller_ ? controller_->get_uuid() : std::string();
}

void InstanceMgr::run_task(typename AsyncRunner<TaskPtr>::Id id, TaskPtr task_ptr)
{
    LogFunc << VAR(id) << VAR(task_ptr);

    const json::value details = {
        { "id", id },
        { "task", std::string(task_ptr->type()) },
        { "uuid", get_controller_uuid() },
        { "hash", get_resource_hash() },
    };

    notify_runner_->append({ .msg = MaaMsg::TaskStarted, .details = details });
    bool ret = task_ptr->run();
    notify_runner_->append({ .msg = ret ? MaaMsg::TaskCompleted : MaaMsg::TaskFailed, .details = details });

    task_map_.erase(id);
}

void InstanceMgr::notify(typename AsyncRunner<NotifyData>::Id id, NotifyData cb_data)
{
    LogFunc << VAR_VOIDP(callback_) << VAR_VOIDP(callback_arg_) << VAR(id) << VAR(cb_data.msg) << VAR(cb_data.details);

    if (callback_) {
        callback_(static_cast<MaaMsgId>(id), cb_data.details.to_string().c_str(), callback_arg_);
    }
}

MAA_NS_END
