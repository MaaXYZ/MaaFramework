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

MAA_TOOLKIT_NS_END
