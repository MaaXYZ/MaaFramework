#include "CustomControllerAgent.h"

#include "Buffer/ImageBuffer.hpp"
#include "Buffer/StringBuffer.hpp"
#include "MaaFramework/Instance/MaaCustomController.h"
#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

MAA_CTRL_NS_BEGIN

CustomControllerAgent::CustomControllerAgent(MaaCustomControllerHandle handle, MaaTransparentArg handle_arg,
                                             MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg)
    : ControllerAgent(callback, callback_arg), handle_(handle), handle_arg_(handle_arg)
{}

bool CustomControllerAgent::_connect()
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->connect);

    if (!handle_ || !handle_->connect) {
        LogError << "handle_ or handle_->connect is nullptr";
        return false;
    }

    return handle_->connect(handle_arg_);
}

std::optional<std::string> CustomControllerAgent::_request_uuid()
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->request_uuid);

    if (!handle_ || !handle_->request_uuid) {
        LogError << "handle_ or handle_->request_uuid is nullptr";
        return std::nullopt;
    }

    StringBuffer buffer;
    MaaBool ret = handle_->request_uuid(handle_arg_, &buffer);
    if (!ret) {
        LogError << "failed to request_uuid" << VAR(ret);
        return std::nullopt;
    }
    return buffer.get();
}

std::optional<std::pair<int, int>> CustomControllerAgent::_request_resolution()
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->request_resolution);
    if (!handle_ || !handle_->request_resolution) {
        LogError << "handle_ or handle_->request_resolution is nullptr";
        return {};
    }
    int width = 0, height = 0;
    handle_->request_resolution(handle_arg_, &width, &height);
    return std::make_pair(width, height);
}

bool CustomControllerAgent::_start_app(AppParam param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->start_app) << VAR(param.package);

    if (!handle_ || !handle_->start_app) {
        LogError << "handle_ or handle_->start_app is nullptr";
        return false;
    }

    return handle_->start_app(param.package.c_str(), handle_arg_);
}

bool CustomControllerAgent::_stop_app(AppParam param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->stop_app) << VAR(param.package);

    if (!handle_ || !handle_->stop_app) {
        LogError << "handle_ or handle_->stop_app is nullptr";
        return false;
    }

    return handle_->stop_app(param.package.c_str(), handle_arg_);
}

std::optional<cv::Mat> CustomControllerAgent::_screencap()
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->screencap);

    if (!handle_ || !handle_->screencap) {
        LogError << "handle_ or handle_->screencap is nullptr";
        return std::nullopt;
    }

    ImageBuffer buffer;
    MaaBool ret = handle_->screencap(handle_arg_, &buffer);
    if (!ret) {
        LogError << "failed to get_image" << VAR(ret);
        return std::nullopt;
    }

    return buffer.get();
}

bool CustomControllerAgent::_click(ClickParam param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->click) << VAR(param.x) << VAR(param.y);

    if (!handle_ || !handle_->click) {
        LogError << "handle_ or handle_->click is nullptr";
        return false;
    }

    return handle_->click(param.x, param.y, handle_arg_);
}

bool CustomControllerAgent::_swipe(SwipeParam param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->swipe) << VAR(param.x1) << VAR(param.x2) << VAR(param.y1)
            << VAR(param.y2) << VAR(param.duration);

    if (!handle_ || !handle_->swipe) {
        LogError << "handle_ or handle_->swipe is nullptr";
        return false;
    }
    return handle_->swipe(param.x1, param.y1, param.x2, param.y2, param.duration, handle_arg_);
}

bool CustomControllerAgent::_touch_down(TouchParam param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->touch_down) << VAR(param.contact) << VAR(param.x)
            << VAR(param.y) << VAR(param.pressure);

    if (!handle_ || !handle_->touch_down) {
        LogError << "handle_ or handle_->touch_down is nullptr";
        return false;
    }

    return handle_->touch_down(param.contact, param.x, param.y, param.pressure, handle_arg_);
}

bool CustomControllerAgent::_touch_move(TouchParam param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->touch_move) << VAR(param.contact) << VAR(param.x)
            << VAR(param.y) << VAR(param.pressure);

    if (!handle_ || !handle_->touch_move) {
        LogError << "handle_ or handle_->touch_move is nullptr";
        return false;
    }

    return handle_->touch_move(param.contact, param.x, param.y, param.pressure, handle_arg_);
}

bool CustomControllerAgent::_touch_up(TouchParam param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->touch_up) << VAR(param.contact);

    if (!handle_ || !handle_->touch_up) {
        LogError << "handle_ or handle_->touch_up is nullptr";
        return false;
    }

    return handle_->touch_up(param.contact, handle_arg_);
}

bool CustomControllerAgent::_press_key(PressKeyParam param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->press_key) << VAR(param.keycode);

    if (!handle_ || !handle_->press_key) {
        LogError << "handle_ or handle_->press_key is nullptr";
        return false;
    }

    return handle_->press_key(param.keycode, handle_arg_);
}

MAA_CTRL_NS_END
