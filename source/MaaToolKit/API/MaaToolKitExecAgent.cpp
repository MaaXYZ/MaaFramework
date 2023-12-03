#include "MaaToolKit/ExecAgent/MaaToolKitExecAgent.h"

#include <string_view>

#include <meojson/json.hpp>

#include "ExecAgent/ActionExecAgent.h"
#include "ExecAgent/RecognizerExecAgent.h"
#include "Utils/Logger.h"

using namespace MAA_TOOLKIT_NS;

enum class ExecutorType
{
    Recognizer,
    Action,
};
std::ostream& operator<<(std::ostream& os, ExecutorType type)
{
    switch (type) {
    case ExecutorType::Recognizer:
        os << "Recognizer";
        break;
    case ExecutorType::Action:
        os << "Action";
        break;
    }
    return os;
}

MaaBool RegisterExecutor(ExecutorType type, MaaInstanceHandle handle, MaaStringView name, MaaStringView exec_path,
                         MaaStringView exec_param_json, MaaToolKitExecAgentArgvTransferMode argv_mode)
{
    LogFunc << VAR(type) << VAR_VOIDP(handle) << VAR(name) << VAR(exec_path) << VAR(exec_param_json) << VAR(argv_mode);

    if (!handle) {
        LogError << "handle is null";
        return false;
    }
    if (std::string_view(name).empty()) {
        LogError << "name is empty";
        return false;
    }

    auto path = MAA_NS::path(exec_path);
    if (!std::filesystem::exists(path)) {
        LogError << "exec path not exists:" << path;
        return false;
    }

    auto params_opt = json::parse(exec_param_json);
    if (!params_opt) {
        LogError << "exec param json parse failed:" << exec_param_json;
        return false;
    }
    if (!params_opt->is_array() ||
        !MAA_RNS::ranges::all_of(params_opt->as_array(), [](const auto& v) { return v.is_string(); })) {
        LogError << "exec param json is not array of string:" << exec_param_json;
        return false;
    }
    auto params = params_opt->to_vector<std::string>();

    TextTransferMode text_mode =
        static_cast<TextTransferMode>(MaaToolKitExecAgentArgvTransferMode_Text_Mask & argv_mode);
    ImageTransferMode image_mode =
        static_cast<ImageTransferMode>(MaaToolKitExecAgentArgvTransferMode_Image_Mask & argv_mode);

    switch (type) {
    case ExecutorType::Recognizer:
        return RecognizerExecAgent::get_instance().register_executor( //
            handle, name, std::move(path), std::move(params), text_mode, image_mode);
    case ExecutorType::Action:
        return ActionExecAgent::get_instance().register_executor( //
            handle, name, std::move(path), std::move(params), text_mode, image_mode);
    }

    return false;
}

MaaBool UnregisterExecutor(ExecutorType type, MaaInstanceHandle handle, MaaStringView name)
{
    LogFunc << VAR(type) << VAR_VOIDP(handle) << VAR(name);

    if (!handle) {
        LogError << "handle is null";
        return false;
    }
    if (std::string_view(name).empty()) {
        LogError << "name is empty";
        return false;
    }

    switch (type) {
    case ExecutorType::Recognizer:
        return RecognizerExecAgent::get_instance().unregister_executor(handle, name);

    case ExecutorType::Action:
        return ActionExecAgent::get_instance().unregister_executor(handle, name);
    }

    return false;
}

MaaBool MaaToolKitRegisterCustomRecognizerExecutor( //
    MaaInstanceHandle handle, MaaStringView recognizer_name, MaaStringView recognizer_exec_path,
    MaaStringView recognizer_exec_param_json, MaaToolKitExecAgentArgvTransferMode argv_mode)
{
    return RegisterExecutor(ExecutorType::Recognizer, handle, recognizer_name, recognizer_exec_path,
                            recognizer_exec_param_json, argv_mode);
}

MaaBool MaaToolKitUnregisterCustomRecognizerExecutor(MaaInstanceHandle handle, MaaStringView recognizer_name)
{
    return UnregisterExecutor(ExecutorType::Recognizer, handle, recognizer_name);
}

MaaBool MaaToolKitRegisterCustomActionExecutor( //
    MaaInstanceHandle handle, MaaStringView action_name, MaaStringView action_exec_path,
    MaaStringView action_exec_param_json, MaaToolKitExecAgentArgvTransferMode argv_mode)
{
    return RegisterExecutor(ExecutorType::Action, handle, action_name, action_exec_path, action_exec_param_json,
                            argv_mode);
}

MaaBool MaaToolKitUnregisterCustomActionExecutor(MaaInstanceHandle handle, MaaStringView action_name)
{
    return UnregisterExecutor(ExecutorType::Action, handle, action_name);
}
