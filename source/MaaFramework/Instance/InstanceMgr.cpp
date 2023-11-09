#include "InstanceMgr.h"

#include "Controller/ControllerAgent.h"
#include "MaaFramework/MaaMsg.h"
#include "Resource/ResourceMgr.h"
#include "Task/CustomAction.h"
#include "Task/PipelineTask.h"
#include "Utils/Logger.h"
#include "Vision/CustomRecognizer.h"

MAA_NS_BEGIN

InstanceMgr::InstanceMgr(MaaInstanceCallback callback, MaaCallbackTransparentArg callback_arg)
    : notifier(callback, callback_arg)
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
}

bool InstanceMgr::bind_resource(MaaResourceAPI* resource)
{
    LogInfo << VAR_VOIDP(this) << VAR_VOIDP(resource);

    if (!resource) {
        LogError << "Invalid resource";
        return false;
    }

    if (!resource->loaded()) {
        LogWarn << "Resource not loaded";
    }

    if (resource_) {
        LogWarn << "Resource already binded" << VAR_VOIDP(resource_);
    }

    resource_ = resource;
    return true;
}

bool InstanceMgr::bind_controller(MaaControllerAPI* controller)
{
    LogInfo << VAR_VOIDP(this) << VAR_VOIDP(controller);

    if (!controller) {
        LogError << "Invalid controller";
        return false;
    }

    if (!controller->connected()) {
        LogWarn << "Controller not connected";
    }

    if (controller_) {
        LogWarn << "Controller already binded" << VAR_VOIDP(controller_);
    }

    controller_ = controller;
    return true;
}

bool InstanceMgr::inited() const
{
    return resource_ && controller_ && resource_->loaded() && controller_->connected();
}

bool InstanceMgr::set_option(MaaInstOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    std::ignore = key;
    std::ignore = value;
    std::ignore = val_size;

    return false;
}

MaaTaskId InstanceMgr::post_task(std::string entry, std::string_view param)
{
    LogInfo << VAR(entry) << VAR(param);

    TaskPtr task_ptr = std::make_shared<TaskNS::PipelineTask>(std::move(entry), this);

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

    if (!task_runner_) {
        LogError << "task_runner is nullptr";
        return MaaInvalidId;
    }
    auto id = task_runner_->post(task_ptr);
    LogDebug << VAR(id);

    return id;
}

bool InstanceMgr::set_task_param(MaaTaskId task_id, std::string_view param)
{
    LogInfo << VAR(task_id) << VAR(param);

    auto param_opt = json::parse(param);
    if (!param_opt) {
        LogError << "Invalid param:" << param;
        return false;
    }

    bool ret = false;
    task_runner_->for_each([&](TaskId id, TaskPtr task_ptr) {
        if (static_cast<TaskId>(task_id) != id) {
            return;
        }
        ret = task_ptr->set_param(*param_opt);
        LogDebug << VAR(id) << VAR(ret);
    });

    return ret;
}

bool InstanceMgr::register_custom_recognizer(std::string name, MaaCustomRecognizerHandle handle,
                                             MaaTransparentArg handle_arg)
{
    LogInfo << VAR(name) << VAR_VOIDP(handle) << VAR_VOIDP(handle_arg);
    if (!handle) {
        LogError << "Invalid handle";
        return false;
    }

    auto recognizer_ptr = std::make_shared<MAA_VISION_NS::CustomRecognizer>(handle, handle_arg, this);
    return custom_recognizers_.insert_or_assign(std::move(name), std::move(recognizer_ptr)).second;
}

bool InstanceMgr::unregister_custom_recognizer(std::string name)
{
    LogInfo << VAR(name);
    return custom_recognizers_.erase(name) > 0;
}

void InstanceMgr::clear_custom_recognizer()
{
    LogInfo;
    custom_recognizers_.clear();
}

bool InstanceMgr::register_custom_action(std::string name, MaaCustomActionHandle handle, MaaTransparentArg handle_arg)
{
    LogInfo << VAR(name) << VAR_VOIDP(handle) << VAR_VOIDP(handle_arg);
    if (!handle) {
        LogError << "Invalid handle";
        return false;
    }
    auto action_ptr = std::make_shared<MAA_TASK_NS::CustomAction>(handle, handle_arg, this);
    return custom_actions_.insert_or_assign(std::move(name), std::move(action_ptr)).second;
}

bool InstanceMgr::unregister_custom_action(std::string name)
{
    LogInfo << VAR(name);
    return custom_actions_.erase(name) > 0;
}

void InstanceMgr::clear_custom_action()
{
    LogInfo;
    custom_actions_.clear();
}

MaaStatus InstanceMgr::task_status(MaaTaskId task_id) const
{
    if (!task_runner_) {
        LogError << "task_runner is nullptr";
        return MaaStatus_Invalid;
    }
    return task_runner_->status(task_id);
}

MaaStatus InstanceMgr::task_wait(MaaTaskId task_id) const
{
    if (!task_runner_) {
        LogError << "task_runner is nullptr";
        return MaaStatus_Invalid;
    }
    task_runner_->wait(task_id);
    return task_runner_->status(task_id);
}

MaaBool InstanceMgr::task_all_finished() const
{
    if (!task_runner_) {
        LogError << "task_runner is nullptr";
        return false;
    }
    return !task_runner_->running();
}

void InstanceMgr::stop()
{
    LogFunc;

    if (resource_) {
        resource_->on_stop();
    }
    if (controller_) {
        controller_->on_stop();
    }

    task_runner_->for_each([](TaskId id, TaskPtr task_ptr) {
        std::ignore = id;
        if (task_ptr) {
            task_ptr->on_stop();
        }
    });
    task_runner_->clear();
}

MaaResourceHandle InstanceMgr::resource()
{
    return resource_;
}

MaaControllerHandle InstanceMgr::controller()
{
    return controller_;
}

MAA_RES_NS::ResourceMgr* InstanceMgr::inter_resource()
{
    return dynamic_cast<MAA_RES_NS::ResourceMgr*>(resource());
}

MAA_CTRL_NS::ControllerAgent* InstanceMgr::inter_controller()
{
    return dynamic_cast<MAA_CTRL_NS::ControllerAgent*>(controller());
}

InstanceStatus* InstanceMgr::inter_status()
{
    return &status_;
}

void InstanceMgr::notify(std::string_view msg, const json::value& details)
{
    notifier.notify(msg, details);
}

MAA_VISION_NS::CustomRecognizerPtr InstanceMgr::custom_recognizer(const std::string& name)
{
    auto it = custom_recognizers_.find(name);
    if (it == custom_recognizers_.end()) {
        LogError << "Custom recognizer not found:" << name;
        return nullptr;
    }
    return it->second;
}

MAA_TASK_NS::CustomActionPtr InstanceMgr::custom_action(const std::string& name)
{
    auto it = custom_actions_.find(name);
    if (it == custom_actions_.end()) {
        LogError << "Custom action not found:" << name;
        return nullptr;
    }
    return it->second;
}

bool InstanceMgr::run_task(TaskId id, TaskPtr task_ptr)
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
        { "hash", inter_resource() ? inter_resource()->get_hash() : std::string() },
        { "uuid", inter_controller() ? inter_controller()->get_uuid() : std::string() },
    };

    notifier.notify(MaaMsg_Task_Started, details);

    LogInfo << "task start:" << VAR(details);

    task_ptr->set_taskid(id);
    bool ret = task_ptr->run();

    LogInfo << "task end:" << VAR(details) << VAR(ret);

    notifier.notify(ret ? MaaMsg_Task_Completed : MaaMsg_Task_Failed, details);

    status_.clear();

    MAA_LOG_NS::Logger::get_instance().flush();

    return ret;
}

MAA_NS_END
