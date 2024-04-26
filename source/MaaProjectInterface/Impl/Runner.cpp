#include "ProjectInterface/Runner.h"

#include <format>
#include <iostream>

#include <meojson/json.hpp>

#include "MaaFramework/MaaAPI.h"
#include "MaaToolkit/MaaToolkitAPI.h"

#include "Utils/Logger.h"
#include "Utils/ScopeLeave.hpp"

MAA_PROJECT_INTERFACE_NS_BEGIN

bool Runner::run(
    const RuntimeParam& param,
    MaaInstanceCallback callback,
    MaaCallbackTransparentArg callback_arg,
    MaaResourceCallback resource_callback,
    MaaCallbackTransparentArg resource_callback_arg,
    MaaControllerCallback controller_callback,
    MaaCallbackTransparentArg controller_callback_arg)
{
    auto maa_handle = MaaCreate(callback, callback_arg);

    MaaControllerHandle controller_handle = nullptr;
    if (const auto* p_adb_param = std::get_if<RuntimeParam::AdbParam>(&param.controller_param)) {
        controller_handle = MaaAdbControllerCreateV2(
            p_adb_param->adb_path.c_str(),
            p_adb_param->address.c_str(),
            p_adb_param->controller_type,
            p_adb_param->config.c_str(),
            p_adb_param->agent_path.c_str(),
            controller_callback,
            controller_callback_arg);
    }
    else if (
        const auto* p_win32_param =
            std::get_if<RuntimeParam::Win32Param>(&param.controller_param)) {
        controller_handle = MaaWin32ControllerCreate(
            p_win32_param->hwnd,
            p_win32_param->controller_type,
            controller_callback,
            controller_callback_arg);
    }
    else {
        LogError << "Unknown controller type";
        return false;
    }

    auto resource_handle = MaaResourceCreate(resource_callback, resource_callback_arg);

    int64_t cid = MaaControllerPostConnection(controller_handle);
    int64_t rid = 0;
    for (const auto& path : param.resource_path) {
        rid = MaaResourcePostPath(resource_handle, path.c_str());
    }

    MaaBindResource(maa_handle, resource_handle);
    MaaBindController(maa_handle, controller_handle);

    if (MaaStatus_Failed == MaaControllerWait(controller_handle, cid)) {
        LogError << "Failed to connect controller";
        return false;
    }

    if (MaaStatus_Failed == MaaResourceWait(resource_handle, rid)) {
        LogError << "Failed to load resource";
        return false;
    }

    OnScopeLeave([&]() {
        MaaDestroy(maa_handle);
        MaaResourceDestroy(resource_handle);
        MaaControllerDestroy(controller_handle);
    });

    for (const auto& [name, executor] : param.recognizer) {
        std::vector<const char*> exec_params;
        for (const auto& p : executor.exec_param) {
            exec_params.push_back(p.c_str());
        }
        MaaToolkitRegisterCustomRecognizerExecutor(
            maa_handle,
            name.c_str(),
            executor.exec_path.c_str(),
            exec_params.data(),
            exec_params.size());
    }
    for (const auto& [name, executor] : param.action) {
        std::vector<const char*> exec_params;
        for (const auto& p : executor.exec_param) {
            exec_params.push_back(p.c_str());
        }
        MaaToolkitRegisterCustomActionExecutor(
            maa_handle,
            name.c_str(),
            executor.exec_path.c_str(),
            exec_params.data(),
            exec_params.size());
    }

    int64_t tid = 0;
    for (const auto& task : param.task) {
        std::string task_param = task.param.to_string();
        tid = MaaPostTask(maa_handle, task.entry.c_str(), task_param.c_str());
    }

    MaaWaitTask(maa_handle, tid);

    return true;
}

MAA_PROJECT_INTERFACE_NS_END
