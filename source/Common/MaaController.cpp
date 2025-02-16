#include "MaaFramework/MaaAPI.h"

#include "Common/MaaTypes.h"
#include "Utils/Buffer/ImageBuffer.hpp"
#include "Utils/Buffer/StringBuffer.hpp"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

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
