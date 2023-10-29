#include "MaaFramework/Instance/MaaController.h"

#include "Buffer/ImageBuffer.hpp"
#include "Buffer/StringBuffer.hpp"
#include "ControlUnit/AdbControlUnitAPI.h"
#include "Controller/AdbController.h"
#include "Controller/CustomController.h"
#include "Controller/CustomThriftController.h"
#include "Controller/DebuggingController.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MaaControllerHandle MaaAdbControllerCreate(MaaStringView adb_path, MaaStringView address, MaaAdbControllerType type,
                                           MaaStringView config, MaaControllerCallback callback,
                                           MaaCallbackTransparentArg callback_arg)
{
    LogWarn << "This API" << __FUNCTION__
            << "is about to be deprecated, and we recommend using MaaAdbControllerCreateV2 instead.";

    constexpr std::string_view kDefaultAgentPath = "./MaaAgentBinary";
    return MaaAdbControllerCreateV2(adb_path, address, type, config, kDefaultAgentPath.data(), callback, callback_arg);
}

MaaControllerHandle MaaAdbControllerCreateV2(MaaStringView adb_path, MaaStringView address, MaaAdbControllerType type,
                                             MaaStringView config, MaaStringView agent_path,
                                             MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR(adb_path) << VAR(address) << VAR(type) << VAR(agent_path) << VAR_VOIDP(callback)
            << VAR_VOIDP(callback_arg);

#ifdef WITH_ADB_CONTROLLER

    auto unit_mgr = MAA_ADB_CTRL_UNIT_NS::create_controller_unit(adb_path, address, type, config, agent_path);
    if (!unit_mgr) {
        LogError << "Failed to create controller unit";
        return nullptr;
    }

    return new MAA_CTRL_NS::AdbController(adb_path, address, std::move(unit_mgr), callback, callback_arg);

#else

#pragma message("The build without adb controller")

    std::ignore = config;

    LogError << "The build without adb controller";
    return nullptr;

#endif // WITH_ADB_CONTROLLER
}

MaaControllerHandle MaaCustomControllerCreate(MaaCustomControllerHandle handle, MaaTransparentArg handle_arg,
                                              MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR(handle) << VAR(handle_arg) << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return new MAA_CTRL_NS::CustomController(handle, handle_arg, callback, callback_arg);
}

MaaControllerHandle MaaThriftControllerCreate(MaaStringView param, MaaControllerCallback callback,
                                              MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR(param) << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

#ifdef WITH_THRIFT_CONTROLLER

    try {
        return new MAA_CTRL_NS::CustomThriftController(param, callback, callback_arg);
    }
    catch (const std::exception& e) {
        LogError << "Failed to create thrift controller: " << e.what();
        return nullptr;
    }

#else

#pragma message("The build without thrift controller")

    LogError << "The build without thrift controller";
    return nullptr;

#endif // WITH_THRIFT_CONTROLLER
}

MaaControllerHandle MaaDbgControllerCreate(MaaStringView read_path, MaaStringView write_path, MaaDbgControllerType type,
                                           MaaStringView config, MaaControllerCallback callback,
                                           MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR(read_path) << VAR(write_path) << VAR(type) << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

#ifdef WITH_DBG_CONTROLLER

    auto unit_mgr = MAA_DBG_CTRL_UNIT_NS::create_controller(type, read_path, write_path, config);
    if (!unit_mgr) {
        LogError << "Failed to create controller unit";
        return nullptr;
    }

    return new MAA_CTRL_NS::DebuggingController(read_path, write_path, std::move(unit_mgr), callback, callback_arg);

#else

#pragma message("The build without debugging controller")

    std::ignore = config;

    LogError << "The build without debugging controller";
    return nullptr;

#endif // WITH_DBG_CONTROLLER
}

void MaaControllerDestroy(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (ctrl == nullptr) {
        LogError << "handle is null";
        return;
    }

    ctrl->on_stop();
    delete ctrl;
}

MaaBool MaaControllerSetOption(MaaControllerHandle ctrl, MaaCtrlOption key, MaaOptionValue value,
                               MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!ctrl) {
        LogError << "handle is null";
        return false;
    }

    return ctrl->set_option(key, value, val_size);
}

MaaCtrlId MaaControllerPostConnection(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (!ctrl) {
        LogError << "handle is null";
        return false;
    }

    return ctrl->post_connection();
}

MaaCtrlId MaaControllerPostClick(MaaControllerHandle ctrl, int32_t x, int32_t y)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(x) << VAR(y);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_click(x, y);
}

MaaCtrlId MaaControllerPostSwipe(MaaControllerHandle ctrl, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                                 int32_t duration)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_swipe(x1, y1, x2, y2, duration);
}

MaaCtrlId MaaControllerPostPressKey(MaaControllerHandle ctrl, int32_t keycode)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(keycode);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_press_key(keycode);
}

MaaCtrlId MaaControllerPostTouchDown(MaaControllerHandle ctrl, int32_t contact, int32_t x, int32_t y, int32_t pressure)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_touch_down(contact, x, y, pressure);
}

MaaCtrlId MaaControllerPostTouchMove(MaaControllerHandle ctrl, int32_t contact, int32_t x, int32_t y, int32_t pressure)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_touch_move(contact, x, y, pressure);
}

MaaCtrlId MaaControllerPostTouchUp(MaaControllerHandle ctrl, int32_t contact)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(contact);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_touch_up(contact);
}

MaaCtrlId MaaControllerPostScreencap(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_screencap();
}

MaaStatus MaaControllerStatus(MaaControllerHandle ctrl, MaaCtrlId id)
{
    // LogFunc << VAR_VOIDP(ctrl) << VAR(id);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return ctrl->status(id);
}

MaaStatus MaaControllerWait(MaaControllerHandle ctrl, MaaCtrlId id)
{
    // LogFunc << VAR_VOIDP(ctrl) << VAR(id);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return ctrl->wait(id);
}

MaaBool MaaControllerConnected(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (!ctrl) {
        LogError << "handle is null";
        return false;
    }

    return ctrl->connected();
}

MaaBool MaaControllerGetImage(MaaControllerHandle ctrl, MaaImageBufferHandle buffer)
{
    if (!ctrl || !buffer) {
        LogError << "handle is null";
        return false;
    }

    auto img = ctrl->get_image();
    if (img.empty()) {
        LogError << "image is empty";
        return false;
    }

    buffer->set(std::move(img));
    return true;
}

MaaBool MaaControllerGetUUID(MaaControllerHandle ctrl, MaaStringBufferHandle buffer)
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
