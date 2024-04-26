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
    MaaStringView* exec_params,
    MaaSize exec_param_size)
{
    LogFunc << VAR(type) << VAR_VOIDP(handle) << VAR(name) << VAR(exec_path) << VAR(exec_params)
            << VAR(exec_param_size);

    if (!handle) {
        LogError << "handle is null";
        return false;
    }
    if (!exec_params && exec_param_size != 0) {
        LogError << "invalid exec params";
        return false;
    }

    if (std::string_view(name).empty()) {
        LogError << "name is empty";
        return false;
    }

    auto path = MAA_NS::path(exec_path);

    std::vector<std::string> params;
    for (size_t i = 0; i < exec_param_size; ++i) {
        const char* p = *(exec_params + i);
        params.emplace_back(p);
    }

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

MaaBool ClearExecutor(ExecutorType type, MaaInstanceHandle handle)
{
    LogFunc << VAR(type) << VAR_VOIDP(handle);

    if (!handle) {
        LogError << "handle is null";
        return false;
    }

    switch (type) {
    case ExecutorType::Recognizer:
        return RecognizerExecAgent::get_instance().unregister_executor(handle, "");

    case ExecutorType::Action:
        return ActionExecAgent::get_instance().unregister_executor(handle, "");
    }

    return false;
}

MaaBool MaaToolkitRegisterCustomRecognizerExecutor(
    MaaInstanceHandle handle,
    MaaStringView recognizer_name,
    MaaStringView exec_path,
    MaaStringView* exec_params,
    MaaSize exec_param_size)
{
    return RegisterExecutor(
        ExecutorType::Recognizer,
        handle,
        recognizer_name,
        exec_path,
        exec_params,
        exec_param_size);
}

MaaBool MaaToolkitUnregisterCustomRecognizerExecutor(
    MaaInstanceHandle handle,
    MaaStringView recognizer_name)
{
    return UnregisterExecutor(ExecutorType::Recognizer, handle, recognizer_name);
}

MaaBool MaaToolkitClearCustomRecognizerExecutor(MaaInstanceHandle handle)
{
    return ClearExecutor(ExecutorType::Recognizer, handle);
}

MaaBool MaaToolkitRegisterCustomActionExecutor(
    MaaInstanceHandle handle,
    MaaStringView action_name,
    MaaStringView exec_path,
    MaaStringView* exec_params,
    MaaSize exec_param_size)
{
    return RegisterExecutor(
        ExecutorType::Action,
        handle,
        action_name,
        exec_path,
        exec_params,
        exec_param_size);
}

MaaBool
    MaaToolkitUnregisterCustomActionExecutor(MaaInstanceHandle handle, MaaStringView action_name)
{
    return UnregisterExecutor(ExecutorType::Action, handle, action_name);
}

MaaBool MaaToolkitClearCustomActionExecutor(MaaInstanceHandle handle)
{
    return ClearExecutor(ExecutorType::Action, handle);
}
