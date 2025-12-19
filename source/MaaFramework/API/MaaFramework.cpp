#include "MaaFramework/MaaAPI.h"

#include "Controller/ControllerAgent.h"
#include "Global/OptionMgr.h"
#include "Global/PluginMgr.h"
#include "LibraryHolder/ControlUnit.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/Tasker.h"

MaaController* MaaAdbControllerCreate(
    const char* adb_path,
    const char* address,
    MaaAdbScreencapMethod screencap_methods,
    MaaAdbInputMethod input_methods,
    const char* config,
    const char* agent_path)
{
    LogFunc << VAR(adb_path) << VAR(address) << VAR(screencap_methods) << VAR(input_methods) << VAR(config) << VAR(agent_path);

    if (!adb_path || !address) {
        LogError << "adb_path or address is null";
        return nullptr;
    }

    if (!config) {
        LogError << "config is null";
        return nullptr;
    }

    if (!agent_path) {
        LogError << "agent_path is null";
        return nullptr;
    }

    auto control_unit =
        MAA_NS::AdbControlUnitLibraryHolder::create_control_unit(adb_path, address, screencap_methods, input_methods, config, agent_path);

    if (!control_unit) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return new MAA_CTRL_NS::ControllerAgent(std::move(control_unit));
}

MaaController* MaaWin32ControllerCreate(
    void* hWnd,
    MaaWin32ScreencapMethod screencap_method,
    MaaWin32InputMethod mouse_method,
    MaaWin32InputMethod keyboard_method)
{
    LogFunc << VAR_VOIDP(hWnd) << VAR(screencap_method) << VAR(mouse_method) << VAR(keyboard_method);

#ifndef _WIN32

    LogError << "This API" << __FUNCTION__ << "is only available on Windows";
    return nullptr;

#else

    if (!hWnd) {
        LogWarn << "hWnd is nullptr";
    }

    auto control_unit = MAA_NS::Win32ControlUnitLibraryHolder::create_control_unit(hWnd, screencap_method, mouse_method, keyboard_method);

    if (!control_unit) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return new MAA_CTRL_NS::ControllerAgent(std::move(control_unit));
#endif
}

MaaController* MaaCustomControllerCreate(MaaCustomControllerCallbacks* controller, void* controller_arg)
{
    LogFunc << VAR(controller) << VAR(controller_arg);

    if (!controller) {
        LogError << "controller is null";
        return nullptr;
    }

    auto control_unit = MAA_NS::CustomControlUnitLibraryHolder::create_control_unit(controller, controller_arg);

    if (!control_unit) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return new MAA_CTRL_NS::ControllerAgent(std::move(control_unit));
}

MaaController* MaaDbgControllerCreate(const char* read_path, const char* write_path, MaaDbgControllerType type, const char* config)
{
    LogFunc << VAR(read_path) << VAR(write_path) << VAR(type);

    if (!read_path) {
        LogError << "read_path is null";
        return nullptr;
    }

    std::ignore = write_path;
    std::ignore = config;

    auto control_unit = MAA_NS::DbgControlUnitLibraryHolder::create_control_unit(type, read_path);

    if (!control_unit) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return new MAA_CTRL_NS::ControllerAgent(std::move(control_unit));
}

MaaController* MaaPlayCoverControllerCreate(const char* address, const char* uuid)
{
    LogFunc << VAR(address) << VAR(uuid);

    if (!address) {
        LogError << "address is null";
        return nullptr;
    }

    if (!uuid) {
        LogError << "uuid is null";
        return nullptr;
    }

    auto control_unit = MAA_NS::PlayCoverControlUnitLibraryHolder::create_control_unit(address, uuid);

    if (!control_unit) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return new MAA_CTRL_NS::ControllerAgent(std::move(control_unit));
}

void MaaControllerDestroy(MaaController* ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (ctrl == nullptr) {
        LogError << "handle is null";
        return;
    }

    delete ctrl;
}

MaaResource* MaaResourceCreate()
{
    LogFunc;

    return new MAA_RES_NS::ResourceMgr;
}

void MaaResourceDestroy(MaaResource* res)
{
    LogFunc << VAR_VOIDP(res);

    if (res == nullptr) {
        LogError << "handle is null";
        return;
    }

    delete res;
}

MaaTasker* MaaTaskerCreate()
{
    LogFunc;

    return new MAA_NS::Tasker;
}

void MaaTaskerDestroy(MaaTasker* tasker)
{
    LogFunc << VAR_VOIDP(tasker);

    if (tasker == nullptr) {
        LogError << "handle is null";
        return;
    }

    delete tasker;
}

MaaBool MaaGlobalSetOption(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    return MAA_GLOBAL_NS::OptionMgr::get_instance().set_option(key, value, val_size);
}

MaaBool MaaGlobalLoadPlugin(const char* library_path)
{
    LogFunc << VAR(library_path);

    if (!library_path) {
        LogError << "library_path is null";
        return false;
    }

    return MAA_GLOBAL_NS::PluginMgr::get_instance().load(MAA_NS::path(library_path));
}

MaaBool MaaSetGlobalOption(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogError << "Deprecated API, use MaaGlobalSetOption instead";

    return MaaGlobalSetOption(key, value, val_size);
}
