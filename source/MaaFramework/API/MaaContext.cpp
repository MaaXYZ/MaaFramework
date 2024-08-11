#include "MaaFramework/Instance/MaaContext.h"

#include "API/MaaTypes.h"
#include "Buffer/ImageBuffer.hpp"
#include "Buffer/StringBuffer.hpp"
#include "Utils/Logger.h"

MaaBool MaaContextRunTask(
    MaaContext* context,
    const char* task_name,
    const char* param)
{
    LogFunc << VAR_VOIDP(context) << VAR(task_name) << VAR(param);

    if (!context) {
        LogError << "handle is null";
        return false;
    }

    return context->run_task(task_name, param);
}

MaaBool MaaContextRunRecognition(
    MaaContext* context,
    MaaImageBuffer* image,
    const char* task_name,
    const char* task_param,
    MaaRect* out_box,
    MaaStringBuffer* out_detail)
{
    LogFunc << VAR_VOIDP(context) << VAR_VOIDP(image) << VAR(task_name) << VAR(task_param)
            << VAR(out_box) << VAR(out_detail);

    if (!context || !image) {
        LogError << "handle is null";
        return false;
    }

    cv::Rect cvbox {};
    std::string detail;

    bool ret = context->run_recognition(image->get(), task_name, task_param, cvbox, detail);

    if (out_box) {
        out_box->x = cvbox.x;
        out_box->y = cvbox.y;
        out_box->width = cvbox.width;
        out_box->height = cvbox.height;
    }
    if (out_detail) {
        out_detail->set(std::move(detail));
    }

    return ret;
}

MaaBool MaaContextRunAction(
    MaaContext* context,
    const char* task_name,
    const char* task_param,
    MaaRect* cur_box,
    const char* cur_rec_detail)
{
    LogFunc << VAR_VOIDP(context) << VAR(task_name) << VAR(task_param) << VAR(cur_box)
            << VAR(cur_rec_detail);

    if (!context) {
        LogError << "handle is null";
        return false;
    }

    cv::Rect cvbox {};
    if (cur_box) {
        cvbox.x = cur_box->x;
        cvbox.y = cur_box->y;
        cvbox.width = cur_box->width;
        cvbox.height = cur_box->height;
    }

    bool ret = context->run_action(task_name, task_param, cvbox, cur_rec_detail);
    return ret;
}

MaaBool MaaContextClick(MaaContext* context, int32_t x, int32_t y)
{
    LogFunc << VAR_VOIDP(context) << VAR(x) << VAR(y);

    if (!context) {
        LogError << "handle is null";
        return false;
    }

    return context->click(x, y);
}

MaaBool MaaContextSwipe(
    MaaContext* context,
    int32_t x1,
    int32_t y1,
    int32_t x2,
    int32_t y2,
    int32_t duration)
{
    LogFunc << VAR_VOIDP(context) << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    if (!context) {
        LogError << "handle is null";
        return false;
    }

    return context->swipe(x1, y1, x2, y2, duration);
}

MaaBool MaaContextPressKey(MaaContext* context, int32_t keycode)
{
    LogFunc << VAR_VOIDP(context) << VAR(keycode);

    if (!context) {
        LogError << "handle is null";
        return false;
    }

    return context->press_key(keycode);
}

MaaBool MaaContextInputText(MaaContext* context, const char* text)
{
    LogFunc << VAR_VOIDP(context) << VAR(text);

    if (!context) {
        LogError << "handle is null";
        return false;
    }

    return context->input_text(text);
}

MaaBool MaaContextTouchDown(
    MaaContext* context,
    int32_t contact,
    int32_t x,
    int32_t y,
    int32_t pressure)
{
    LogFunc << VAR_VOIDP(context) << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    if (!context) {
        LogError << "handle is null";
        return false;
    }

    return context->touch_down(contact, x, y, pressure);
}

MaaBool MaaContextTouchMove(
    MaaContext* context,
    int32_t contact,
    int32_t x,
    int32_t y,
    int32_t pressure)
{
    LogFunc << VAR_VOIDP(context) << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    if (!context) {
        LogError << "handle is null";
        return false;
    }

    return context->touch_move(contact, x, y, pressure);
}

MaaBool MaaContextTouchUp(MaaContext* context, int32_t contact)
{
    LogFunc << VAR_VOIDP(context) << VAR(contact);

    if (!context) {
        LogError << "handle is null";
        return false;
    }

    return context->touch_up(contact);
}

MaaBool MaaContextScreencap(MaaContext* context, MaaImageBuffer* out_image)
{
    LogFunc << VAR_VOIDP(context) << VAR(out_image);

    if (!context || !out_image) {
        LogError << "handle is null";
        return false;
    }

    auto img = context->screencap();
    if (img.empty()) {
        LogError << "image is empty";
        return false;
    }

    out_image->set(std::move(img));
    return true;
}

MaaBool MaaContextCachedImage(MaaContext* context, MaaImageBuffer* out_image)
{
    LogFunc << VAR_VOIDP(context) << VAR(out_image);

    if (!context || !out_image) {
        LogError << "handle is null";
        return false;
    }

    auto img = context->cached_image();
    if (img.empty()) {
        LogError << "image is empty";
        return false;
    }

    out_image->set(std::move(img));
    return true;
}
