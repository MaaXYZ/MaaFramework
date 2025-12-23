#include "ProjectInterface/Runner.h"

#include <format>
#include <iostream>

#include <meojson/json.hpp>

#include "MaaAgentClient/MaaAgentClientAPI.h"
#include "MaaFramework/MaaAPI.h"
#include "MaaToolkit/MaaToolkitAPI.h"

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

RuntimeParam::AdbParam reconfig_adb(const RuntimeParam::AdbParam& raw)
{
    auto list_handle = MaaToolkitAdbDeviceListCreate();
    OnScopeLeave([&]() { MaaToolkitAdbDeviceListDestroy(list_handle); });

    MaaToolkitAdbDeviceFind(list_handle);

    size_t size = MaaToolkitAdbDeviceListSize(list_handle);
    for (size_t i = 0; i < size; ++i) {
        auto device_handle = MaaToolkitAdbDeviceListAt(list_handle, i);

        std::string name = MaaToolkitAdbDeviceGetName(device_handle);
        std::string path = MaaToolkitAdbDeviceGetAdbPath(device_handle);

        if (name != raw.name || path != raw.adb_path) {
            continue;
        }

        LogInfo << "Reconfigure ADB Param" << VAR(name) << VAR(path);

        std::string new_address = MaaToolkitAdbDeviceGetAddress(device_handle);
        if (raw.address != new_address) {
            LogInfo << "ADB Address changed" << VAR(raw.address) << VAR(new_address);
        }

        RuntimeParam::AdbParam new_param = raw;
        new_param.address = new_address;
        new_param.input = MaaToolkitAdbDeviceGetInputMethods(device_handle);
        new_param.screencap = MaaToolkitAdbDeviceGetScreencapMethods(device_handle);
        new_param.config = MaaToolkitAdbDeviceGetConfig(device_handle);
        return new_param;
    }

    return raw;
}

bool Runner::run(const RuntimeParam& param)
{
    MaaTasker* tasker_handle = MaaTaskerCreate();

    MaaController* controller_handle = nullptr;
    if (const auto* p_adb_param = std::get_if<RuntimeParam::AdbParam>(&param.controller_param)) {
        RuntimeParam::AdbParam adb_param = reconfig_adb(*p_adb_param);
        controller_handle = MaaAdbControllerCreate(
            adb_param.adb_path.c_str(),
            adb_param.address.c_str(),
            adb_param.screencap,
            adb_param.input,
            adb_param.config.c_str(),
            adb_param.agent_path.c_str());
    }
    else if (const auto* p_win32_param = std::get_if<RuntimeParam::Win32Param>(&param.controller_param)) {
        controller_handle =
            MaaWin32ControllerCreate(p_win32_param->hwnd, p_win32_param->screencap, p_win32_param->mouse, p_win32_param->keyboard);
    }
    else if (const auto* p_playcover_param = std::get_if<RuntimeParam::PlayCoverParam>(&param.controller_param)) {
#if defined(__APPLE__)
        controller_handle = MaaPlayCoverControllerCreate(p_playcover_param->address.c_str(), p_playcover_param->uuid.c_str());
#else
        std::ignore = p_playcover_param;
        LogError << "PlayCover controller is only supported on macOS";
        return false;
#endif
    }
    else {
        LogError << "Unknown controller type";
        return false;
    }

    MaaResource* resource_handle = MaaResourceCreate();

    OnScopeLeave([&]() {
        MaaTaskerDestroy(tasker_handle);
        MaaResourceDestroy(resource_handle);
        MaaControllerDestroy(controller_handle);
    });

    // 设置分辨率选项
    if (param.display_config.raw) {
        MaaBool raw = true;
        MaaControllerSetOption(controller_handle, MaaCtrlOption_ScreenshotUseRawSize, &raw, sizeof(raw));
    }
    else if (param.display_config.long_side.has_value()) {
        int long_side = param.display_config.long_side.value();
        MaaControllerSetOption(controller_handle, MaaCtrlOption_ScreenshotTargetLongSide, &long_side, sizeof(long_side));
    }
    else if (param.display_config.short_side.has_value()) {
        int short_side = param.display_config.short_side.value();
        MaaControllerSetOption(controller_handle, MaaCtrlOption_ScreenshotTargetShortSide, &short_side, sizeof(short_side));
    }
    // 如果都没设置，使用默认值 720
    else {
        int short_side = 720;
        MaaControllerSetOption(controller_handle, MaaCtrlOption_ScreenshotTargetShortSide, &short_side, sizeof(short_side));
    }

    MaaId cid = controller_handle->post_connection();
    MaaId rid = 0;
    for (const auto& path : param.resource_path) {
        rid = resource_handle->post_bundle(path);
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
        auto os_args = conv_args(args);

        LogInfo << "Start Agent" << VAR(param.agent->child_exec) << VAR(os_args) << VAR(param.agent->cwd);
        agent_child = boost::process::child(param.agent->child_exec, os_args, boost::process::start_dir = param.agent->cwd);
        if (!agent_child.valid()) {
            LogError << "Failed to start agent process" << VAR(param.agent->child_exec) << VAR(args) << VAR(param.agent->cwd);
            return false;
        }

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
