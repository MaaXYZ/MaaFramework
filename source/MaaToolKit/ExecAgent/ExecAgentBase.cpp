#include "ExecAgentBase.h"

#include "MaaFramework/MaaAPI.h"
#include "Utils/Boost.hpp"
#include "Utils/IOStream/ChildPipeIOStream.h"
#include "Utils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

bool ExecAgentBase::register_executor(MaaInstanceHandle handle, std::string_view name, std::filesystem::path exec_path,
                                      std::vector<std::string> exec_args, TextTransferMode text_mode,
                                      ImageTransferMode image_mode)
{
    LogFunc << VAR_VOIDP(handle) << VAR(name) << VAR(exec_path) << VAR(exec_args) << VAR(text_mode) << VAR(image_mode);

    bool registered = register_for_maa_inst(handle, name);
    if (!registered) {
        LogError << "register failed" << VAR(name);
        return false;
    }

    Executor executor {
        .name = std::string(name),
        .exec_path = std::move(exec_path),
        .exec_args = std::move(exec_args),
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

std::optional<std::string> ExecAgentBase::run_executor(TextTransferMode mode, const std::filesystem::path& exec_path,
                                                       const std::vector<std::string>& exec_args)
{
    switch (mode) {
    case TextTransferMode::StdIO: {
        return run_executor_with_stdio(exec_path, exec_args);
    }
    case TextTransferMode::FileIO:
    default:
        LogError << "not implemented";
        return std::nullopt;
    }
}

std::optional<std::string> ExecAgentBase::run_executor_with_stdio(const std::filesystem::path& exec_path,
                                                                  const std::vector<std::string>& exec_args)
{
    auto searched_path = boost::process::search_path(exec_path);
    if (!std::filesystem::exists(searched_path)) {
        LogError << "path not exists" << VAR(searched_path);
        return std::nullopt;
    }

    ChildPipeIOStream ios(searched_path, exec_args);

    while (ios.is_open()) {
        std::string line = ios.read_until("\n");
        LogInfo << "read line:" << line;

        // TODO
    }

    int exit_code = ios.release();
    if (exit_code != 0) {
        LogError << "process exit with code:" << exit_code;
        return std::nullopt;
    }

    // TODO
    return {};
}

MAA_TOOLKIT_NS_END
