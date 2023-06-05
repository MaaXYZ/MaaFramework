#include "InstanceMgr.h"

MAA_NS_BEGIN

InstanceMgr::InstanceMgr(MaaInstanceCallback callback, void* callback_arg)
    : callback_(callback), callback_arg_(callback_arg)
{
    task_runner_ = std::make_unique<AsyncRunner<TaskPtr>>(
        std::bind(&InstanceMgr::run_task, this, std::placeholders::_1, std::placeholders::_2));
    notify_runner_ = std::make_unique<AsyncRunner<NotifyData>>(
        std::bind(&InstanceMgr::notify, this, std::placeholders::_1, std::placeholders::_2));
}

InstanceMgr::~InstanceMgr() {}

bool InstanceMgr::bind_resource(MaaResourceAPI* resource)
{
    return false;
}

bool InstanceMgr::bind_controller(MaaControllerAPI* controller)
{
    return false;
}

bool InstanceMgr::inited() const
{
    return false;
}

bool InstanceMgr::set_option(std::string_view key, std::string_view value)
{
    return false;
}

MaaTaskId InstanceMgr::append_task(std::string_view type, const std::string& param)
{
    return MaaTaskId();
}

bool InstanceMgr::set_task_param(MaaTaskId task_id, const std::string& param)
{
    return false;
}

std::vector<MaaTaskId> InstanceMgr::get_task_list() const
{
    return std::vector<MaaTaskId>();
}

bool InstanceMgr::start()
{
    return false;
}

bool InstanceMgr::stop()
{
    return false;
}

bool InstanceMgr::running() const
{
    return false;
}

std::string InstanceMgr::get_resource_hash() const
{
    return std::string();
}

std::string InstanceMgr::get_controller_uuid() const
{
    return std::string();
}

void InstanceMgr::run_task(AsyncRunner<TaskPtr>::Id id, TaskPtr task_ptr)
{
    LogFunc << VAR(id) << VAR(task_ptr);

    const json::value details = {
        { "id", id },
        { "task", std::string(task_ptr->info()) },
        { "uuid", get_controller_uuid() },
        { "hash", get_resource_hash() },
    };

    notify_runner_->append({ .msg = MaaMsg::TaskStarted, .details = details });
    bool ret = task_ptr->run();
    notify_runner_->append({ .msg = ret ? MaaMsg::TaskCompleted : MaaMsg::TaskFailed, .details = details });
}

void InstanceMgr::notify(AsyncRunner<NotifyData>::Id id, NotifyData cb_data)
{
    LogFunc << VAR(id) << VAR(cb_data.msg) << VAR(cb_data.details);

    callback_(static_cast<MaaMsgId>(id), cb_data.details.to_string().c_str(), callback_arg_);
}

MAA_NS_END
