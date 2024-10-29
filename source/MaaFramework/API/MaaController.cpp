#include "MaaFramework/Instance/MaaController.h"

#include "Controller/CustomControllerAgent.h"
#include "Controller/GeneralControllerAgent.h"
#include "LibraryHolder/ControlUnit.h"
#include "Utils/Buffer/ImageBuffer.hpp"
#include "Utils/Buffer/StringBuffer.hpp"
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

    return new MAA_CTRL_NS::GeneralControllerAgent(std::move(control_unit), notify, notify_trans_arg);
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

    return new MAA_CTRL_NS::GeneralControllerAgent(std::move(control_unit), notify, notify_trans_arg);

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

    return new MAA_CTRL_NS::CustomControllerAgent(controller, controller_arg, notify, notify_trans_arg);
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

    return new MAA_CTRL_NS::GeneralControllerAgent(std::move(control_unit), notify, notify_trans_arg);
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

MaaBool MaaControllerSetOption(MaaController* ctrl, MaaCtrlOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!ctrl) {
        LogError << "handle is null";
        return false;
    }

    return ctrl->set_option(key, value, val_size);
}

MaaCtrlId MaaControllerPostConnection(MaaController* ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (!ctrl) {
        LogError << "handle is null";
        return false;
    }

    return ctrl->post_connection();
}

MaaCtrlId MaaControllerPostClick(MaaController* ctrl, int32_t x, int32_t y)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(x) << VAR(y);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_click(x, y);
}

MaaCtrlId MaaControllerPostSwipe(MaaController* ctrl, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_swipe(x1, y1, x2, y2, duration);
}

MaaCtrlId MaaControllerPostPressKey(MaaController* ctrl, int32_t keycode)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(keycode);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_press_key(keycode);
}

MaaCtrlId MaaControllerPostInputText(MaaController* ctrl, const char* text)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(text);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_input_text(text);
}

MaaCtrlId MaaControllerPostStartApp(MaaController* ctrl, const char* intent)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(intent);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_start_app(intent);
}

MaaCtrlId MaaControllerPostStopApp(MaaController* ctrl, const char* intent)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(intent);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_stop_app(intent);
}

MaaCtrlId MaaControllerPostTouchDown(MaaController* ctrl, int32_t contact, int32_t x, int32_t y, int32_t pressure)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_touch_down(contact, x, y, pressure);
}

MaaCtrlId MaaControllerPostTouchMove(MaaController* ctrl, int32_t contact, int32_t x, int32_t y, int32_t pressure)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_touch_move(contact, x, y, pressure);
}

MaaCtrlId MaaControllerPostTouchUp(MaaController* ctrl, int32_t contact)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(contact);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_touch_up(contact);
}

MaaCtrlId MaaControllerPostScreencap(MaaController* ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_screencap();
}

MaaStatus MaaControllerStatus(const MaaController* ctrl, MaaCtrlId id)
{
    // LogFunc << VAR_VOIDP(ctrl) << VAR(id);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return ctrl->status(id);
}

MaaStatus MaaControllerWait(const MaaController* ctrl, MaaCtrlId id)
{
    // LogFunc << VAR_VOIDP(ctrl) << VAR(id);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return ctrl->wait(id);
}

MaaBool MaaControllerConnected(const MaaController* ctrl)
{
    if (!ctrl) {
        LogError << "handle is null";
        return false;
    }

    return ctrl->connected();
}

MaaBool MaaControllerCachedImage(const MaaController* ctrl, MaaImageBuffer* buffer)
{
    if (!ctrl || !buffer) {
        LogError << "handle is null";
        return false;
    }

    auto img = ctrl->cached_image();
    if (img.empty()) {
        LogError << "image is empty";
        return false;
    }

    buffer->set(std::move(img));
    return true;
}

MaaBool MaaControllerGetUuid(MaaController* ctrl, MaaStringBuffer* buffer)
{
    if (!ctrl || !buffer) {
        LogError << "handle is null";
        return false;
    }

    auto uuid = ctrl->get_uuid();
    if (uuid.empty()) {
        LogError << "uuid is empty";
        return false;
    }

    buffer->set(std::move(uuid));
    return true;
}
