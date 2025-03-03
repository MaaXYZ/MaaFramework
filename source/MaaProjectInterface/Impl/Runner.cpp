#include "ProjectInterface/Runner.h"

#include <format>
#include <iostream>

#include <meojson/json.hpp>

#include "MaaAgentClient/MaaAgentClientAPI.h"
#include "MaaFramework/MaaAPI.h"

#include "Common/MaaTypes.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/ScopeLeave.hpp"

MAA_PROJECT_INTERFACE_NS_BEGIN

bool Runner::run(
    const RuntimeParam& param,
    MaaNotificationCallback notify,
    void* notify_trans_arg,
    const std::map<std::string, CustomRecognitionSession>& custom_recognitions,
    const std::map<std::string, CustomActionSession>& custom_actions)
{
    MaaTasker* tasker_handle = MaaTaskerCreate(notify, notify_trans_arg);

    MaaController* controller_handle = nullptr;
    if (const auto* p_adb_param = std::get_if<RuntimeParam::AdbParam>(&param.controller_param)) {
        controller_handle = MaaAdbControllerCreate(
            p_adb_param->adb_path.c_str(),
            p_adb_param->address.c_str(),
            p_adb_param->screencap,
            p_adb_param->input,
            p_adb_param->config.c_str(),
            p_adb_param->agent_path.c_str(),
            notify,
            notify_trans_arg);
    }
    else if (const auto* p_win32_param = std::get_if<RuntimeParam::Win32Param>(&param.controller_param)) {
        controller_handle =
            MaaWin32ControllerCreate(p_win32_param->hwnd, p_win32_param->screencap, p_win32_param->input, notify, notify_trans_arg);
    }
    else {
        LogError << "Unknown controller type";
        return false;
    }

    MaaResource* resource_handle = MaaResourceCreate(notify, notify_trans_arg);
    resource_handle->set_option(MaaResOption_InferenceDevice, const_cast<int32_t*>(&param.gpu), sizeof(int32_t));

    MaaAgentClient* agent_handle = nullptr;
    if (param.agent) {
        agent_handle = MaaAgentClientCreate();
        agent_handle->bind_resource(resource_handle);
        agent_handle->start_clild(param.agent->child_exec, param.agent->child_args);
    }

    OnScopeLeave([&]() {
        MaaAgentClientDestroy(agent_handle);
        MaaTaskerDestroy(tasker_handle);
        MaaResourceDestroy(resource_handle);
        MaaControllerDestroy(controller_handle);
    });

    MaaId cid = controller_handle->post_connection();
    MaaId rid = 0;
    for (const auto& path : param.resource_path) {
        rid = resource_handle->post_bundle(path);
    }
    for (const auto& [name, reco] : custom_recognitions) {
        resource_handle->register_custom_recognition(name, reco.recognition, reco.trans_arg);
    }
    for (const auto& [name, act] : custom_actions) {
        resource_handle->register_custom_action(name, act.action, act.trans_arg);
    }

    tasker_handle->bind_controller(controller_handle);
    tasker_handle->bind_resource(resource_handle);

    if (MaaStatus_Failed == controller_handle->wait(cid)) {
        LogError << "Failed to connect controller";
        return false;
    }

    if (MaaStatus_Failed == resource_handle->wait(rid)) {
        LogError << "Failed to load resource";
        return false;
    }

    MaaId tid = 0;
    for (const auto& task : param.task) {
        tid = tasker_handle->post_task(task.entry, task.pipeline_override);
    }

    tasker_handle->wait(rid);

    return true;
}

MAA_PROJECT_INTERFACE_NS_END
