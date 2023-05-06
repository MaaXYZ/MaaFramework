#include "InstanceMgr.h"

MAA_NS_BEGIN

InstanceMgr::InstanceMgr(const std::filesystem::path& user_path, MaaInstanceCallback callback, void* callback_arg)
    : user_path_(user_path), callback_(callback), callback_arg_(callback_arg)
{}

InstanceMgr::~InstanceMgr() {}

MaaInstanceAPI* InstanceMgr::create_ex(const std::filesystem::path& user_path, MaaInstanceCallback callback,
                                       void* callback_arg)
{
    return new InstanceMgr(user_path, callback, callback_arg);
}

void InstanceMgr::destroy(MaaInstanceAPI** handle_ptr)
{
    if (handle_ptr == nullptr || *handle_ptr == nullptr) {
        return;
    }

    auto* inst = dynamic_cast<InstanceMgr*>(*handle_ptr);
    delete inst;
    *handle_ptr = nullptr;
}

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
