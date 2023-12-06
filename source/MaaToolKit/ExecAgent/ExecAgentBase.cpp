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

    auto [iter, inserted] = exec_data_.insert_or_assign( //
        std::string(name), ExecData {
                               .name = std::string(name),
                               .exec_path = std::move(exec_path),
                               .exec_args = std::move(exec_args),
                               .text_mode = text_mode,
                               .image_mode = image_mode,
                           });
    if (!inserted || iter == exec_data_.end()) {
        LogError << "insert failed" << VAR(name);
    }

    bool registered = register_for_maa_inst(handle, name, iter->second);
    if (!registered) {
        LogError << "register failed" << VAR(name);
        exec_data_.erase(iter);
        return false;
    }

    return true;
}

bool ExecAgentBase::unregister_executor(MaaInstanceHandle handle, std::string_view name)
{
    LogFunc << VAR_VOIDP(handle) << VAR(name);

    bool ret = unregister_for_maa_inst(handle, name);
    ret &= exec_data_.erase(std::string(name)) > 0;

    return ret;
}

std::optional<std::string> ExecAgentBase::run_executor(const std::filesystem::path& exec_path,
                                                       const std::vector<std::string>& exec_args,
                                                       TextTransferMode text_mode, ImageTransferMode image_mode)
{
    auto searched_path = boost::process::search_path(exec_path);
    if (!std::filesystem::exists(searched_path)) {
        LogError << "path not exists" << VAR(searched_path);
        return std::nullopt;
    }

    ChildPipeIOStream child(searched_path, exec_args);

    switch (text_mode) {
    case TextTransferMode::StdIO:
        return handle_ipc(child, image_mode);

    case TextTransferMode::FileIO:
        LogError << "not implemented";
        return std::nullopt;

    default:
        LogError << "not implemented";
        return std::nullopt;
    }
}

std::optional<std::string> ExecAgentBase::handle_ipc(IOStream& ios, ImageTransferMode image_mode)
{
    std::ignore = image_mode;

    while (ios.is_open()) {
        std::string line = ios.read_until("\n");
        LogInfo << "read line:" << line;

        // TODO
    }

    if (!ios.release()) {
        return std::nullopt;
    }

    // TODO
    return {};
}

MAA_TOOLKIT_NS_END
