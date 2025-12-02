#include "MaaFramework/MaaAPI.h"

#include "Common/MaaTypes.h"
#include "MaaUtils/Buffer/ImageBuffer.hpp"
#include "MaaUtils/Buffer/StringBuffer.hpp"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MaaSinkId MaaControllerAddSink(MaaController* ctrl, MaaEventCallback sink, void* trans_arg)
{
    LogInfo << VAR_VOIDP(ctrl) << VAR_VOIDP(sink) << VAR_VOIDP(trans_arg);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->add_sink(sink, trans_arg);
}

void MaaControllerRemoveSink(MaaController* ctrl, MaaSinkId sink_id)
{
    LogInfo << VAR_VOIDP(ctrl) << VAR(sink_id);

    if (!ctrl) {
        LogError << "handle is null";
        return;
    }

    ctrl->remove_sink(sink_id);
}

void MaaControllerClearSinks(MaaController* ctrl)
{
    LogInfo << VAR_VOIDP(ctrl);

    if (!ctrl) {
        LogError << "handle is null";
        return;
    }

    ctrl->clear_sinks();
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
    LogError << "Deprecated API, use MaaControllerPostClickKey instead";

    return MaaControllerPostClickKey(ctrl, keycode);
}

MaaCtrlId MaaControllerPostClickKey(MaaController* ctrl, int32_t keycode)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(keycode);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_click_key(keycode);
}

MaaCtrlId MaaControllerPostInputText(MaaController* ctrl, const char* text)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(text);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    if (!text) {
        LogError << "text is null";
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

    if (!intent) {
        LogError << "intent is null";
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

    if (!intent) {
        LogError << "intent is null";
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

MaaCtrlId MaaControllerPostKeyDown(MaaController* ctrl, int32_t keycode)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(keycode);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_key_down(keycode);
}

MaaCtrlId MaaControllerPostKeyUp(MaaController* ctrl, int32_t keycode)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(keycode);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_key_up(keycode);
}

MaaCtrlId MaaControllerPostScroll(MaaController* ctrl, int32_t dx, int32_t dy)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(dx) << VAR(dy);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_scroll(dx, dy);
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

#define CheckNullAndWarn(var)                        \
    if (!var) {                                      \
        LogWarn << #var << "is null, no assignment"; \
    }                                                \
    else

MaaBool MaaControllerGetActionDetail(
    const MaaController* ctrl,
    MaaActId action_id,
    MaaStringBuffer* name,
    MaaStringBuffer* action,
    MaaRect* box,
    MaaBool* success,
    MaaStringBuffer* detail_json)
{
    if (!ctrl) {
        LogError << "handle is null";
        return false;
    }

    auto result_opt = ctrl->get_action_result(action_id);
    if (!result_opt) {
        LogError << "failed to get_action_result" << VAR(action_id);
        return false;
    }

    auto& result = *result_opt;

    CheckNullAndWarn(name)
    {
        name->set(result.name);
    }
    CheckNullAndWarn(action)
    {
        action->set(result.action);
    }
    CheckNullAndWarn(box)
    {
        box->x = result.box.x;
        box->y = result.box.y;
        box->width = result.box.width;
        box->height = result.box.height;
    }
    CheckNullAndWarn(success)
    {
        *success = result.success;
    }
    CheckNullAndWarn(detail_json)
    {
        detail_json->set(result.detail.to_string());
    }

    return true;
}

#undef CheckNullAndWarn

MaaBool MaaControllerClearActionCache(MaaController* ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (!ctrl) {
        LogError << "handle is null";
        return false;
    }

    ctrl->clear_action_cache();
    return true;
}
