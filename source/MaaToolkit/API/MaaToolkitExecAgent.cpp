#include "MaaToolkit/ExecAgent/MaaToolkitExecAgent.h"

#include <string_view>

#include <meojson/json.hpp>

#include "ExecAgent/ActionExecAgent.h"
#include "ExecAgent/RecognizerExecAgent.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

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

MaaBool RegisterExecutor(
    ExecutorType type,
    MaaInstanceHandle handle,
    MaaStringView name,
    MaaStringView exec_path,
    MaaStringView exec_param_json)
{
    LogFunc << VAR(type) << VAR_VOIDP(handle) << VAR(name) << VAR(exec_path)
            << VAR(exec_param_json);

    if (!handle) {
        LogError << "handle is null";
        return false;
    }
    if (std::string_view(name).empty()) {
        LogError << "name is empty";
        return false;
    }

    auto path = MAA_NS::path(exec_path);

    auto params_opt = json::parse(exec_param_json);
    if (!params_opt) {
        LogError << "exec param json parse failed:" << exec_param_json;
        return false;
    }
    if (!params_opt->is<std::vector<std::string>>()) {
        LogError << "exec param json is not array of string:" << exec_param_json;
        return false;
    }
    auto params = params_opt->as<std::vector<std::string>>();

    switch (type) {
    case ExecutorType::Recognizer:
        return RecognizerExecAgent::get_instance()
            .register_executor(handle, name, std::move(path), std::move(params));
    case ExecutorType::Action:
        return ActionExecAgent::get_instance()
            .register_executor(handle, name, std::move(path), std::move(params));
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

MaaBool MaaToolkitRegisterCustomRecognizerExecutor(
    MaaInstanceHandle handle,
    MaaStringView recognizer_name,
    MaaStringView recognizer_exec_path,
    MaaStringView recognizer_exec_param_json)
{
    return RegisterExecutor(
        ExecutorType::Recognizer,
        handle,
        recognizer_name,
        recognizer_exec_path,
        recognizer_exec_param_json);
}

MaaBool MaaToolkitUnregisterCustomRecognizerExecutor(
    MaaInstanceHandle handle,
    MaaStringView recognizer_name)
{
    return UnregisterExecutor(ExecutorType::Recognizer, handle, recognizer_name);
}

MaaBool MaaToolkitRegisterCustomActionExecutor(
    MaaInstanceHandle handle,
    MaaStringView action_name,
    MaaStringView action_exec_path,
    MaaStringView action_exec_param_json)
{
    return RegisterExecutor(
        ExecutorType::Action,
        handle,
        action_name,
        action_exec_path,
        action_exec_param_json);
}

MaaBool
    MaaToolkitUnregisterCustomActionExecutor(MaaInstanceHandle handle, MaaStringView action_name)
{
    return UnregisterExecutor(ExecutorType::Action, handle, action_name);
}
