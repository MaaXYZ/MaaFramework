#include "MaaInstance.h"

MAA_NS_BEGIN

MaaInstance::MaaInstance(const std::filesystem::path& user_path, MaaInstanceCallback callback, void* callback_arg)
    : user_path_(user_path), callback_(callback), callback_arg_(callback_arg)
{}

MaaInstance::~MaaInstance() {}

MaaInstanceAPI* MaaInstance::create_ex(const std::filesystem::path& user_path, MaaInstanceCallback callback,
                                       void* callback_arg)
{
    return new MaaInstance(user_path, callback, callback_arg);
}

void MaaInstance::destroy(MaaInstanceAPI** handle_ptr)
{
    if (handle_ptr == nullptr || *handle_ptr == nullptr) {
        return;
    }

    auto* inst = dynamic_cast<MaaInstance*>(*handle_ptr);
    delete inst;
    *handle_ptr = nullptr;
}

bool MaaInstance::bind_resource(MaaResourceAPI* resource)
{
    return false;
}

bool MaaInstance::bind_controller(MaaControllerAPI* controller)
{
    return false;
}

bool MaaInstance::inited() const
{
    return false;
}

bool MaaInstance::set_option(InstanceOptionKey key, const std::string& value)
{
    return false;
}

MaaTaskId MaaInstance::append_task(const std::string& type, const std::string& param)
{
    return MaaTaskId();
}

bool MaaInstance::set_task_param(MaaTaskId task_id, const std::string& param)
{
    return false;
}

std::vector<MaaTaskId> MaaInstance::get_task_list() const
{
    return std::vector<MaaTaskId>();
}

bool MaaInstance::start()
{
    return false;
}

bool MaaInstance::stop()
{
    return false;
}

bool MaaInstance::running() const
{
    return false;
}

std::string MaaInstance::get_resource_hash() const
{
    return std::string();
}

std::string MaaInstance::get_controller_uuid() const
{
    return std::string();
}

MAA_NS_END
