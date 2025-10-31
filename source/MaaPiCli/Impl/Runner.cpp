#include "ProjectInterface/Runner.h"

#include <format>
#include <iostream>

#include <meojson/json.hpp>

#include "MaaAgentClient/MaaAgentClientAPI.h"
#include "MaaFramework/MaaAPI.h"

#include "Common/MaaTypes.h"
#include "MaaUtils/Encoding.h"
#include "MaaUtils/IOStream/BoostIO.hpp"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/ScopeLeave.hpp"

MAA_PROJECT_INTERFACE_NS_BEGIN

#ifdef _WIN32
std::vector<std::wstring> conv_args(const std::vector<std::string>& args)
{
    std::vector<std::wstring> wargs;
    for (const auto& arg : args) {
        wargs.emplace_back(to_u16(arg));
    }
    return wargs;
}
#else
std::vector<std::string> conv_args(const std::vector<std::string>& args)
{
    return args;
}
#endif

bool Runner::run(
    const RuntimeParam& param,
    const std::map<std::string, CustomRecognitionSession>& custom_recognitions,
    const std::map<std::string, CustomActionSession>& custom_actions)
{
    MaaTasker* tasker_handle = MaaTaskerCreate();

    MaaController* controller_handle = nullptr;
    if (const auto* p_adb_param = std::get_if<RuntimeParam::AdbParam>(&param.controller_param)) {
        controller_handle = MaaAdbControllerCreate(
            p_adb_param->adb_path.c_str(),
            p_adb_param->address.c_str(),
            p_adb_param->screencap,
            p_adb_param->input,
            p_adb_param->config.c_str(),
            p_adb_param->agent_path.c_str());
    }
    else if (const auto* p_win32_param = std::get_if<RuntimeParam::Win32Param>(&param.controller_param)) {
        controller_handle = MaaWin32ControllerCreate(p_win32_param->hwnd, p_win32_param->screencap, p_win32_param->input);
    }
    else {
        LogError << "Unknown controller type";
        return false;
    }

    MaaResource* resource_handle = MaaResourceCreate();
    resource_handle->set_option(MaaResOption_InferenceDevice, const_cast<int32_t*>(&param.gpu), sizeof(int32_t));

    OnScopeLeave([&]() {
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

    MaaAgentClient* agent = nullptr;
    boost::process::child agent_child;
    if (param.agent) {
        agent = MaaAgentClientCreateV2(nullptr);
        MaaAgentClientBindResource(agent, resource_handle);
        auto* id_buffer = MaaStringBufferCreate();
        MaaAgentClientIdentifier(agent, id_buffer);
        std::string socket_id = MaaStringBufferGet(id_buffer);
        MaaStringBufferDestroy(id_buffer);

        std::vector<std::string> args = param.agent->child_args;
        args.emplace_back(socket_id);
        std::filesystem::path exec = boost::process::search_path(param.agent->child_exec);
        if (exec.empty()) {
            LogError << "Failed to find agent executable" << VAR(param.agent->child_exec);
            return false;
        }
        auto os_args = conv_args(args);

        LogInfo << "Start Agent" << VAR(exec) << VAR(os_args);
        agent_child = boost::process::child(exec, os_args);

        bool connected = MaaAgentClientConnect(agent);
        if (!connected) {
            LogError << "Failed to connect agent" << VAR(param.agent->child_exec) << VAR(args);
            return false;
        }
    }

    MaaId tid = 0;
    for (const auto& task : param.task) {
        tid = tasker_handle->post_task(task.entry, task.pipeline_override);
    }

    tasker_handle->wait(tid);

    if (agent) {
        MaaAgentClientDisconnect(agent);
        MaaAgentClientDestroy(agent);
    }

    return true;
}

MAA_PROJECT_INTERFACE_NS_END
