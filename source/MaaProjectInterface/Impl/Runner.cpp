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
    MaaNotificationCallback callback,
    void* callback_arg,
    const std::map<std::string, CustomRecognitionSession>& custom_recognitions,
    const std::map<std::string, CustomActionSession>& custom_actions)
{
    auto tasker_handle = MaaTaskerCreate(callback, callback_arg);

    MaaController* controller_handle = nullptr;
    if (const auto* p_adb_param = std::get_if<RuntimeParam::AdbParam>(&param.controller_param)) {
        controller_handle = MaaAdbControllerCreate(
            p_adb_param->adb_path.c_str(),
            p_adb_param->address.c_str(),
            p_adb_param->screencap,
            p_adb_param->input,
            p_adb_param->config.c_str(),
            p_adb_param->agent_path.c_str(),
            callback,
            callback_arg);
    }
    else if (const auto* p_win32_param = std::get_if<RuntimeParam::Win32Param>(&param.controller_param)) {
        controller_handle =
            MaaWin32ControllerCreate(p_win32_param->hwnd, p_win32_param->screencap, p_win32_param->input, callback, callback_arg);
    }
    else {
        LogError << "Unknown controller type";
        return false;
    }

    auto resource_handle = MaaResourceCreate(callback, callback_arg);

    MaaId cid = MaaControllerPostConnection(controller_handle);
    MaaId rid = 0;
    for (const auto& path : param.resource_path) {
        rid = MaaResourcePostPath(resource_handle, path.c_str());
    }
    for (const auto& [name, reco] : custom_recognitions) {
        MaaResourceRegisterCustomRecognition(resource_handle, name.c_str(), reco.recognition, reco.trans_arg);
    }
    for (const auto& [name, act] : custom_actions) {
        MaaResourceRegisterCustomAction(resource_handle, name.c_str(), act.action, act.trans_arg);
    }

    MaaTaskerBindResource(tasker_handle, resource_handle);
    MaaTaskerBindController(tasker_handle, controller_handle);

    if (MaaStatus_Failed == MaaControllerWait(controller_handle, cid)) {
        LogError << "Failed to connect controller";
        return false;
    }

    if (MaaStatus_Failed == MaaResourceWait(resource_handle, rid)) {
        LogError << "Failed to load resource";
        return false;
    }

    OnScopeLeave([&]() {
        MaaTaskerDestroy(tasker_handle);
        MaaResourceDestroy(resource_handle);
        MaaControllerDestroy(controller_handle);
    });

    MaaId tid = 0;
    for (const auto& task : param.task) {
        std::string pp_override = task.pipeline_override.to_string();
        tid = MaaTaskerPostPipeline(tasker_handle, task.entry.c_str(), pp_override.c_str());
    }

    MaaTaskerWait(tasker_handle, tid);

    return true;
}

MAA_PROJECT_INTERFACE_NS_END
