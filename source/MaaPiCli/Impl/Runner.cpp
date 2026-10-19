#include "ProjectInterface/Runner.h"

#include <atomic>
#include <csignal>
#include <cstdlib>
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

std::string reconfig_linux(const RuntimeParam::LinuxParam& raw)
{
    json::object obj { { "screencap_method", raw.screencap },
                       { "input_method", raw.input },
                       { "use_win32_vk_code", raw.use_win32_vk_code } };
    if (raw.screencap == MaaLinuxScreencapMethod_Wlr || raw.input == MaaLinuxInputMethod_Wlr) {
        obj["wlr_socket_path"] = raw.wlr_socket_path;
    }
    if (raw.screencap == MaaLinuxScreencapMethod_PipeWire) {
        if (raw.pipewire_source == "Gamescope") {
            // gamescope 实例的 PipeWire 节点现查; 跳过无截图节点的实例
            auto instance_list = MaaToolkitGamescopeInstanceListCreate();
            OnScopeLeave([&]() { MaaToolkitGamescopeInstanceListDestroy(instance_list); });

            if (!MaaToolkitGamescopeInstanceFindAll(instance_list)) {
                LogError << "Failed to find gamescope instances";
                return "";
            }

            uint32_t pw_node_id = 0;
            for (MaaSize i = 0; i < MaaToolkitGamescopeInstanceListSize(instance_list); ++i) {
                auto instance = MaaToolkitGamescopeInstanceListAt(instance_list, i);
                if (uint32_t node_id = MaaToolkitGamescopeInstanceGetPipeWireNodeId(instance); node_id != 0) {
                    pw_node_id = node_id;
                    break;
                }
            }
            if (pw_node_id == 0) {
                LogError << "No gamescope PipeWire node found";
                return "";
            }
            obj["pw_node_id"] = pw_node_id;
        }
        else {
            // 显示器捕获: 通过 portal 获取 fd + node id
            auto helper_handle = MaaToolkitPortalHelperCreate();
            if (!helper_handle) {
                LogError << "Failed to create portal helper";
                return "";
            }
            if (!MaaToolkitPortalHelperOpenStream(helper_handle)) {
                LogError << "Failed to open stream";
                MaaToolkitPortalHelperDestroy(helper_handle);
                return "";
            }
            obj["pw_socket_fd"] = MaaToolkitPortalHelperGetPipeWireFD(helper_handle);
            obj["pw_node_id"] = MaaToolkitPortalHelperGetPipeWireNodeID(helper_handle);
            MaaToolkitPortalHelperDestroy(helper_handle);
        }
    }
    if (raw.input == MaaLinuxInputMethod_UInput) {
        obj["uinput_screen_width"] = raw.uinput_screen_width;
        obj["uinput_screen_height"] = raw.uinput_screen_height;
    }
    if (raw.input == MaaLinuxInputMethod_Libei) {
        obj["eis_socket_path"] = raw.eis_socket_path;
    }

    return obj.dumps();
}

#if defined(MAA_WITH_IOS_CONTROLLER)
namespace
{
std::atomic<bool> ios_prepare_cancelled = false;
static_assert(std::atomic<bool>::is_always_lock_free);

extern "C" void cancel_ios_prepare(int)
{
    ios_prepare_cancelled.store(true, std::memory_order_relaxed);
}

MaaBool MAA_CALL is_ios_prepare_cancelled(void*)
{
    return ios_prepare_cancelled.load(std::memory_order_relaxed);
}

std::filesystem::path default_ios_ddi_cache()
{
#if defined(_WIN32)
    wchar_t* local = nullptr;
    size_t length = 0;
    if (_wdupenv_s(&local, &length, L"LOCALAPPDATA") != 0) {
        return {};
    }
    const auto root = local ? std::filesystem::path(local) : std::filesystem::path();
    std::free(local);
    if (root.is_absolute()) {
        return root / "MaaPiCli" / "cache" / "ddi";
    }
#else
#if !defined(__APPLE__)
    const auto* xdg = std::getenv("XDG_CACHE_HOME");
    if (xdg && *xdg && std::filesystem::path(xdg).is_absolute()) {
        return std::filesystem::path(xdg) / "MaaPiCli" / "ddi";
    }
#endif
    const auto* home = std::getenv("HOME");
    if (home && *home && std::filesystem::path(home).is_absolute()) {
#if defined(__APPLE__)
        return std::filesystem::path(home) / "Library" / "Caches" / "MaaPiCli" / "ddi";
#else
        return std::filesystem::path(home) / ".cache" / "MaaPiCli" / "ddi";
#endif
    }
#endif
    return { };
}

MaaBool MAA_CALL print_ios_prepare_progress(const char* text, void*)
{
    const auto event = json::parse(text);
    if (event) {
        std::cout << MAA_NS::utf8_to_crt(event->get("message", std::string())) << std::endl;
    }
    return true;
}

bool prepare_ios(const RuntimeParam::IOSParam& ios)
{
    ios_prepare_cancelled.store(false, std::memory_order_relaxed);
    if (!ios.prepare_ddi) {
        return true;
    }
    if (ios.ddi_download_timeout_seconds < 1 || ios.ddi_download_timeout_seconds > 3600) {
        LogError << "iOS DDI download timeout must be between 1 and 3600 seconds";
        return false;
    }
    const auto cache = ios.ddi_cache_directory.empty() ? default_ios_ddi_cache() : MAA_NS::path(ios.ddi_cache_directory);
    if (cache.empty() || !cache.is_absolute()) {
        LogError << "Cannot resolve an absolute iOS DDI cache directory; set ios.ddi_cache_directory";
        return false;
    }
    const auto options = json::value(
                             json::object { { "cache_directory", MAA_NS::path_to_utf8_string(cache) },
                                            { "download_timeout_seconds", ios.ddi_download_timeout_seconds } })
                             .to_string();
    auto* result = MaaStringBufferCreate();
    if (!result) {
        LogError << "Cannot allocate iOS preparation result";
        return false;
    }
    OnScopeLeave([&]() { MaaStringBufferDestroy(result); });
    const auto previous = std::signal(SIGINT, cancel_ios_prepare);
    if (previous == SIG_ERR) {
        LogError << "Cannot install iOS preparation interrupt handler";
        return false;
    }
    OnScopeLeave([&]() { std::signal(SIGINT, previous); });
    std::cout << "Preparing iOS developer image..." << std::endl;
    if (!MaaToolkitIOSDevicePrepare(
            ios.udid.c_str(),
            ios.wifi.empty() ? MaaIOSConnectionMethod_Auto : MaaIOSConnectionMethod_WiFi,
            ios.wifi.c_str(),
            ios.wifi_pairing.get(),
            options.c_str(),
            print_ios_prepare_progress,
            is_ios_prepare_cancelled,
            nullptr,
            result)) {
        const auto status = json::parse(MaaStringBufferGet(result));
        LogError << "iOS developer image preparation failed"
                 << (status ? status->get("error", std::string()) : "Invalid preparation result");
        return false;
    }
    return !is_ios_prepare_cancelled(nullptr);
}
} // namespace
#endif

bool Runner::run(const RuntimeParam& param)
{
#if defined(MAA_WITH_IOS_CONTROLLER)
    if (const auto* ios = std::get_if<RuntimeParam::IOSParam>(&param.controller_param); ios && !prepare_ios(*ios)) {
        return false;
    }
#endif
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
    else if (const auto* p_ios_param = std::get_if<RuntimeParam::IOSParam>(&param.controller_param)) {
#if defined(MAA_WITH_IOS_CONTROLLER)
        const auto connection = p_ios_param->wifi.empty() ? MaaIOSConnectionMethod_Auto : MaaIOSConnectionMethod_WiFi;
        const auto* address = p_ios_param->wifi == "auto" ? nullptr : p_ios_param->wifi.c_str();
        controller_handle =
            MaaIOSControllerCreate(p_ios_param->udid.c_str(), p_ios_param->screencap, connection, address, p_ios_param->wifi_pairing.get());
#else
        std::ignore = p_ios_param;
        LogError << "iOS controller is disabled in this build";
        return false;
#endif
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
    else if (const auto* p_gamepad_param = std::get_if<RuntimeParam::GamepadParam>(&param.controller_param)) {
#if defined(_WIN32)
        controller_handle = MaaGamepadControllerCreate(p_gamepad_param->hwnd, p_gamepad_param->gamepad_type, p_gamepad_param->screencap);
#else
        std::ignore = p_gamepad_param;
        LogError << "Gamepad controller is only supported on Windows";
        return false;
#endif
    }
    else if (const auto* p_macos_param = std::get_if<RuntimeParam::MacOSParam>(&param.controller_param)) {
#if defined(__APPLE__)
        controller_handle = MaaMacOSControllerCreate(p_macos_param->window_id, p_macos_param->screencap, p_macos_param->input);
#else
        std::ignore = p_macos_param;
        LogError << "MacOS controller is only supported on macOS";
        return false;
#endif
    }
    else if (const auto* p_wlroots_param = std::get_if<RuntimeParam::LinuxParam>(&param.controller_param)) {
#if defined(__linux__)
        auto config_json = reconfig_linux(*p_wlroots_param);
        if (config_json.empty()) {
            LogError << "Failed to build Linux controller config";
            return false;
        }
        controller_handle = MaaLinuxControllerCreate(config_json.c_str());
#else
        std::ignore = p_wlroots_param;
        LogError << "WlRoots controller is only supported on Linux";
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
    else if (param.display_config.expand.has_value()) {
        int32_t expand[2] = { param.display_config.expand->at(0), param.display_config.expand->at(1) };
        MaaControllerSetOption(controller_handle, MaaCtrlOption_ScreenshotTargetExpand, expand, sizeof(expand));
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

    std::vector<MaaAgentClient*> agents;
#if MAAUTILS_HAS_BOOST_PROCESS
    std::vector<boost::process::child> agent_children;
    for (const auto& agent_param : param.agent) {
        MaaAgentClient* agent = MaaAgentClientCreateV2(nullptr);
        MaaAgentClientBindResource(agent, resource_handle);
        auto* id_buffer = MaaStringBufferCreate();
        MaaAgentClientIdentifier(agent, id_buffer);
        std::string socket_id = MaaStringBufferGet(id_buffer);
        MaaStringBufferDestroy(id_buffer);

        std::vector<std::string> args = agent_param.child_args;
        args.emplace_back(socket_id);
        auto os_args = conv_args(args);

        // v2.5.0: set PI_* environment variables in current process (child inherits them)
        for (const auto& [key, val] : agent_param.env_vars) {
            boost::this_process::environment()[key] = val;
        }

        LogInfo << "Start Agent" << VAR(agent_param.child_exec) << VAR(os_args) << VAR(agent_param.cwd);
        auto& agent_child = agent_children.emplace_back(agent_param.child_exec, os_args, boost::process::start_dir = agent_param.cwd);
        if (!agent_child.valid()) {
            LogError << "Failed to start agent process" << VAR(agent_param.child_exec) << VAR(args) << VAR(agent_param.cwd);
            return false;
        }

        bool connected = MaaAgentClientConnect(agent);
        if (!connected) {
            LogError << "Failed to connect agent" << VAR(agent_param.child_exec) << VAR(args);
            return false;
        }

        agents.emplace_back(agent);
    }
#else
    if (!param.agent.empty()) {
        LogError << "Boost.Process is not available, cannot start agent";
        return false;
    }
#endif

    MaaId tid = 0;
    for (const auto& task : param.task) {
        tid = tasker_handle->post_task(task.entry, task.pipeline_override);
    }

    tasker_handle->wait(tid);

    for (auto* agent : agents) {
        MaaAgentClientDisconnect(agent);
        MaaAgentClientDestroy(agent);
    }

    return true;
}

MAA_PROJECT_INTERFACE_NS_END
