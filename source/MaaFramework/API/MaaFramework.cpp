#include "MaaFramework/MaaAPI.h"

#include "Controller/ControllerAgent.h"
#include "Global/GlobalOptionMgr.h"
#include "LibraryHolder/ControlUnit.h"
#include "Resource/ResourceMgr.h"
#include "Tasker/Tasker.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MaaController* MaaAdbControllerCreate(
    const char* adb_path,
    const char* address,
    MaaAdbScreencapMethod screencap_methods,
    MaaAdbInputMethod input_methods,
    const char* config,
    const char* agent_path,
    MaaNotificationCallback notify,
    void* notify_trans_arg)
{
    LogFunc << VAR(adb_path) << VAR(address) << VAR(screencap_methods) << VAR(input_methods) << VAR(config) << VAR(agent_path)
            << VAR_VOIDP(notify) << VAR_VOIDP(notify_trans_arg);

    auto control_unit =
        MAA_NS::AdbControlUnitLibraryHolder::create_control_unit(adb_path, address, screencap_methods, input_methods, config, agent_path);

    if (!control_unit) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    auto* handle = new MAA_CTRL_NS::ControllerAgent(std::move(control_unit));
    handle->add_sink(notify, notify_trans_arg);
    return handle;
}

MaaController* MaaWin32ControllerCreate(
    void* hWnd,
    MaaWin32ScreencapMethod screencap_method,
    MaaWin32InputMethod input_method,
    MaaNotificationCallback notify,
    void* notify_trans_arg)
{
    LogFunc << VAR_VOIDP(hWnd) << VAR(screencap_method) << VAR(input_method) << VAR_VOIDP(notify) << VAR_VOIDP(notify_trans_arg);

#ifndef _WIN32

    LogError << "This API" << __FUNCTION__ << "is only available on Windows";
    return nullptr;

#else

    if (!hWnd) {
        LogWarn << "hWnd is nullptr";
    }

    auto control_unit = MAA_NS::Win32ControlUnitLibraryHolder::create_control_unit(hWnd, screencap_method, input_method);

    if (!control_unit) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    auto* handle = new MAA_CTRL_NS::ControllerAgent(std::move(control_unit));
    handle->add_sink(notify, notify_trans_arg);
    return handle;

#endif
}

MaaController* MaaCustomControllerCreate(
    MaaCustomControllerCallbacks* controller,
    void* controller_arg,
    MaaNotificationCallback notify,
    void* notify_trans_arg)
{
    LogFunc << VAR(controller) << VAR(controller_arg) << VAR_VOIDP(notify) << VAR_VOIDP(notify_trans_arg);

    if (!controller) {
        LogError << "controller is null";
        return nullptr;
    }

    auto control_unit = MAA_NS::CustomControlUnitLibraryHolder::create_control_unit(controller, controller_arg);

    if (!control_unit) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    auto* handle = new MAA_CTRL_NS::ControllerAgent(std::move(control_unit));
    handle->add_sink(notify, notify_trans_arg);
    return handle;
}

MaaController* MaaDbgControllerCreate(
    const char* read_path,
    const char* write_path,
    MaaDbgControllerType type,
    const char* config,
    MaaNotificationCallback notify,
    void* notify_trans_arg)
{
    LogFunc << VAR(read_path) << VAR(write_path) << VAR(type) << VAR_VOIDP(notify) << VAR_VOIDP(notify_trans_arg);

    std::ignore = write_path;
    std::ignore = config;

    auto control_unit = MAA_NS::DbgControlUnitLibraryHolder::create_control_unit(type, read_path);

    if (!control_unit) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    auto* handle = new MAA_CTRL_NS::ControllerAgent(std::move(control_unit));
    handle->add_sink(notify, notify_trans_arg);
    return handle;
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

MaaResource* MaaResourceCreate(MaaNotificationCallback notify, void* notify_trans_arg)
{
    LogFunc << VAR_VOIDP(notify) << VAR_VOIDP(notify_trans_arg);

    auto* handle = new MAA_RES_NS::ResourceMgr;
    handle->add_sink(notify, notify_trans_arg);
    return handle;
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

MaaTasker* MaaTaskerCreate(MaaNotificationCallback notify, void* notify_trans_arg)
{
    LogFunc << VAR_VOIDP(notify) << VAR_VOIDP(notify_trans_arg);

    auto* handle = new MAA_NS::Tasker;
    handle->add_sink(notify, notify_trans_arg);
    return handle;
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

MaaBool MaaSetGlobalOption(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    return MAA_NS::GlobalOptionMgr::get_instance().set_option(key, value, val_size);
}
