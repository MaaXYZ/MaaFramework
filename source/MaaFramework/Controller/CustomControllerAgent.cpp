#include "CustomControllerAgent.h"

#include "MaaFramework/Instance/MaaCustomController.h"
#include "Utils/Buffer/ImageBuffer.hpp"
#include "Utils/Buffer/StringBuffer.hpp"
#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

MAA_CTRL_NS_BEGIN

CustomControllerAgent::CustomControllerAgent(
    MaaCustomControllerCallbacks* controller,
    void* controller_arg,
    MaaNotificationCallback notify,
    void* notify_trans_arg)
    : ControllerAgent(notify, notify_trans_arg)
    , controller_(controller)
    , controller_arg_(controller_arg)
{
    LogDebug << VAR_VOIDP(controller_) << VAR_VOIDP(controller_arg_) << VAR_VOIDP(controller->connect)
             << VAR_VOIDP(controller->request_uuid) << VAR_VOIDP(controller->start_app) << VAR_VOIDP(controller->stop_app)
             << VAR_VOIDP(controller->screencap) << VAR_VOIDP(controller->click) << VAR_VOIDP(controller->swipe)
             << VAR_VOIDP(controller->touch_down) << VAR_VOIDP(controller->touch_move) << VAR_VOIDP(controller->touch_up)
             << VAR_VOIDP(controller->press_key) << VAR_VOIDP(controller->input_text);
}

bool CustomControllerAgent::_connect()
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->connect);

    if (!controller_ || !controller_->connect) {
        LogError << "controller_ or controller_->connect is nullptr";
        return false;
    }

    return controller_->connect(controller_arg_);
}

std::optional<std::string> CustomControllerAgent::_request_uuid()
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->request_uuid);

    if (!controller_ || !controller_->request_uuid) {
        LogError << "controller_ or controller_->request_uuid is nullptr";
        return std::nullopt;
    }

    StringBuffer buffer;
    bool ret = controller_->request_uuid(controller_arg_, &buffer);
    if (!ret) {
        LogError << "failed to request_uuid" << VAR(ret);
        return std::nullopt;
    }
    return buffer.get();
}

bool CustomControllerAgent::_start_app(AppParam param)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->start_app) << VAR(param.package);

    if (!controller_ || !controller_->start_app) {
        LogError << "controller_ or controller_->start_app is nullptr";
        return false;
    }

    return controller_->start_app(param.package.c_str(), controller_arg_);
}

bool CustomControllerAgent::_stop_app(AppParam param)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->stop_app) << VAR(param.package);

    if (!controller_ || !controller_->stop_app) {
        LogError << "controller_ or controller_->stop_app is nullptr";
        return false;
    }

    return controller_->stop_app(param.package.c_str(), controller_arg_);
}

std::optional<cv::Mat> CustomControllerAgent::_screencap()
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->screencap);

    if (!controller_ || !controller_->screencap) {
        LogError << "controller_ or controller_->screencap is nullptr";
        return std::nullopt;
    }

    ImageBuffer buffer;
    bool ret = controller_->screencap(controller_arg_, &buffer);
    if (!ret) {
        LogError << "failed to cached_image" << VAR(ret);
        return std::nullopt;
    }

    return buffer.get();
}

bool CustomControllerAgent::_click(ClickParam param)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->click) << VAR(param.x) << VAR(param.y);

    if (!controller_ || !controller_->click) {
        LogError << "controller_ or controller_->click is nullptr";
        return false;
    }

    return controller_->click(param.x, param.y, controller_arg_);
}

bool CustomControllerAgent::_swipe(SwipeParam param)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->swipe) << VAR(param.x1) << VAR(param.x2) << VAR(param.y1) << VAR(param.y2)
            << VAR(param.duration);

    if (!controller_ || !controller_->swipe) {
        LogError << "controller_ or controller_->swipe is nullptr";
        return false;
    }
    return controller_->swipe(param.x1, param.y1, param.x2, param.y2, param.duration, controller_arg_);
}

bool CustomControllerAgent::_multi_swipe(std::vector<SwipeParam> param)
{
    std::ignore = param;

    LogError << "CustomController not support MulitSwipe now, We welcome your PR to implement it!" << VAR(param);

    return false;
}

bool CustomControllerAgent::_touch_down(TouchParam param)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->touch_down) << VAR(param.contact) << VAR(param.x) << VAR(param.y)
            << VAR(param.pressure);

    if (!controller_ || !controller_->touch_down) {
        LogError << "controller_ or controller_->touch_down is nullptr";
        return false;
    }

    return controller_->touch_down(param.contact, param.x, param.y, param.pressure, controller_arg_);
}

bool CustomControllerAgent::_touch_move(TouchParam param)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->touch_move) << VAR(param.contact) << VAR(param.x) << VAR(param.y)
            << VAR(param.pressure);

    if (!controller_ || !controller_->touch_move) {
        LogError << "controller_ or controller_->touch_move is nullptr";
        return false;
    }

    return controller_->touch_move(param.contact, param.x, param.y, param.pressure, controller_arg_);
}

bool CustomControllerAgent::_touch_up(TouchParam param)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->touch_up) << VAR(param.contact);

    if (!controller_ || !controller_->touch_up) {
        LogError << "controller_ or controller_->touch_up is nullptr";
        return false;
    }

    return controller_->touch_up(param.contact, controller_arg_);
}

bool CustomControllerAgent::_press_key(PressKeyParam param)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->press_key) << VAR(param.keycode);

    if (!controller_ || !controller_->press_key) {
        LogError << "controller_ or controller_->press_key is nullptr";
        return false;
    }

    return controller_->press_key(param.keycode, controller_arg_);
}

bool CustomControllerAgent::_input_text(InputTextParam param)
{
    LogFunc << VAR_VOIDP(controller_) << VAR_VOIDP(controller_->input_text) << VAR(param.text);

    if (!controller_ || !controller_->input_text) {
        LogError << "controller_ or controller_->input_text is nullptr";
        return false;
    }

    return controller_->input_text(param.text.c_str(), controller_arg_);
}

MAA_CTRL_NS_END
