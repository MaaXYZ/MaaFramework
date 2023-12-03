#include "ExecAgentBase.h"
#include "MaaFramework/MaaAPI.h"
#include "Utils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

bool ExecAgentBase::register_executor(MaaInstanceHandle handle, std::string_view name, std::filesystem::path exec_path,
                                      std::vector<std::string> exec_params, TextTransferMode text_mode,
                                      ImageTransferMode image_mode)
{
    LogFunc << VAR_VOIDP(handle) << VAR(name) << VAR(exec_path) << VAR(exec_params) << VAR(text_mode)
            << VAR(image_mode);

    bool registered = register_for_maa_inst(handle, name);
    if (!registered) {
        LogError << "register failed" << VAR(name);
        return false;
    }

    Executor executor {
        .handle_uuid = handle_uuid(handle),
        .name = std::string(name),
        .exec_path = std::move(exec_path),
        .exec_params = std::move(exec_params),
        .text_mode = text_mode,
        .image_mode = image_mode,
    };

    bool inserted = executors_.try_emplace(executor.name, std::move(executor)).second;
    if (!inserted) {
        LogError << "executor already registered: " << executor.name;
        return false;
    }

    return true;
}

bool ExecAgentBase::unregister_executor(MaaInstanceHandle handle, std::string_view name)
{
    LogFunc << VAR_VOIDP(handle) << VAR(name);

    std::string name_str(name);

    bool ret = unregister_for_maa_inst(handle, name);
    ret = executors_.erase(name_str) > 0 && ret;

    return ret;
}

std::string ExecAgentBase::handle_uuid(MaaInstanceHandle handle)
{
    if (!handle) {
        return {};
    }

    for (const auto& [uuid, h] : handles_) {
        if (h == handle) {
            return uuid;
        }
    }

    std::string uuid = std::to_string(reinterpret_cast<uintptr_t>(handle));
    handles_.emplace(uuid, handle);
    return uuid;
}

MaaInstanceHandle ExecAgentBase::uuid_handle(const std::string& uuid) const
{
    auto it = handles_.find(uuid);
    if (it == handles_.end()) {
        return nullptr;
    }
    return it->second;
}

MAA_TOOLKIT_NS_END
