#include "InstanceMgr.h"

MAA_NS_BEGIN

InstanceMgr::InstanceMgr(MaaInstanceCallback callback, void* callback_arg)
    : callback_(callback), callback_arg_(callback_arg)
{}

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

bool InstanceMgr::set_option(InstanceOptionKey key, const std::string& value)
{
    return false;
}

MaaTaskId InstanceMgr::append_task(const std::string& type, const std::string& param)
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

MAA_NS_END
