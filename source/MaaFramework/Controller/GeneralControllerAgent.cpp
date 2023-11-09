#include "GeneralControllerAgent.h"

#include "MaaFramework/MaaMsg.h"
#include "Utils/Logger.h"
#include "Utils/StringMisc.hpp"

#include <meojson/json.hpp>

MAA_CTRL_NS_BEGIN

GeneralControllerAgent::GeneralControllerAgent(std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> control_unit,
                                               MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg)
    : ControllerAgent(callback, callback_arg), control_unit_(std::move(control_unit))
{}

bool GeneralControllerAgent::_connect()
{
    LogFunc;

    if (!control_unit_) {
        LogError << "controller is nullptr" << VAR(control_unit_);
        return false;
    }

    if (!control_unit_->connect()) {
        LogError << "controller connect failed";
        return false;
    }

    return true;
}

std::optional<std::string> GeneralControllerAgent::_request_uuid()
{
    if (!control_unit_) {
        LogError << "controller is nullptr" << VAR(control_unit_);
        return std::nullopt;
    }

    std::string uuid;
    if (!control_unit_->request_uuid(uuid)) {
        LogError << "controller request_uuid failed";
        return std::nullopt;
    }

    return uuid;
}

std::optional<std::pair<int, int>> GeneralControllerAgent::_request_resolution()
{
    if (!control_unit_) {
        LogError << "controller is nullptr" << VAR(control_unit_);
        return std::nullopt;
    }

    int width = 0;
    int height = 0;

    if (!control_unit_->request_resolution(width, height)) {
        LogError << "controller request_resolution failed";
        return std::nullopt;
    }

    return std::make_pair(width, height);
}

bool GeneralControllerAgent::_start_app(AppParam param)
{
    if (!control_unit_) {
        LogError << "controller is nullptr" << VAR(control_unit_);
        return false;
    }

    if (!control_unit_->start_app(param.package)) {
        LogError << "controller start failed" << VAR(param.package);
        return false;
    }

    return true;
}

bool GeneralControllerAgent::_stop_app(AppParam param)
{
    if (!control_unit_) {
        LogError << "controller is nullptr" << VAR(control_unit_);
        return false;
    }

    if (!control_unit_->stop_app(param.package)) {
        LogError << "controller stop failed" << VAR(param.package);
        return false;
    }

    return true;
}

std::optional<cv::Mat> GeneralControllerAgent::_screencap()
{
    if (!control_unit_) {
        LogError << "controller is nullptr" << VAR(control_unit_);
        return std::nullopt;
    }

    cv::Mat image;
    if (!control_unit_->screencap(image)) {
        LogError << "controller screencap failed";
        return std::nullopt;
    }

    // if (image.empty()) {
    //     LogError << "controller screencap empty";
    //     return std::nullopt;
    // }

    return image;
}

bool GeneralControllerAgent::_click(ClickParam param)
{
    if (!control_unit_) {
        LogError << "controller is nullptr" << VAR(control_unit_);
        return false;
    }

    if (!control_unit_->click(param.x, param.y)) {
        LogError << "controller click failed" << VAR(param.x) << VAR(param.y);
        return false;
    }

    return true;
}

bool GeneralControllerAgent::_swipe(SwipeParam param)
{
    if (!control_unit_) {
        LogError << "controller is nullptr" << VAR(control_unit_);
        return false;
    }

    if (!control_unit_->swipe(param.x1, param.y1, param.x2, param.y2, param.duration)) {
        LogError << "controller swipe failed" << VAR(param.x1) << VAR(param.y1) << VAR(param.x2) << VAR(param.y2)
                 << VAR(param.duration);
        return false;
    }

    return true;
}

bool GeneralControllerAgent::_touch_down(TouchParam param)
{
    if (!control_unit_) {
        LogError << "controller is nullptr" << VAR(control_unit_);
        return false;
    }

    if (!control_unit_->touch_down(param.contact, param.x, param.y, param.pressure)) {
        LogError << "controller touch_down failed" << VAR(param.contact) << VAR(param.x) << VAR(param.y)
                 << VAR(param.pressure);
        return false;
    }

    return true;
}

bool GeneralControllerAgent::_touch_move(TouchParam param)
{
    if (!control_unit_) {
        LogError << "controller is nullptr" << VAR(control_unit_);
        return false;
    }

    if (!control_unit_->touch_move(param.contact, param.x, param.y, param.pressure)) {
        LogError << "controller touch_move failed" << VAR(param.contact) << VAR(param.x) << VAR(param.y)
                 << VAR(param.pressure);
        return false;
    }

    return true;
}

bool GeneralControllerAgent::_touch_up(TouchParam param)
{
    if (!control_unit_) {
        LogError << "controller is nullptr" << VAR(control_unit_);
        return false;
    }

    if (!control_unit_->touch_up(param.contact)) {
        LogError << "controller touch_up failed" << VAR(param.contact);
        return false;
    }

    return true;
}

bool GeneralControllerAgent::_press_key(PressKeyParam param)
{
    if (!control_unit_) {
        LogError << "controller is nullptr" << VAR(control_unit_);
        return false;
    }

    if (!control_unit_->press_key(param.keycode)) {
        LogError << "controller press_key failed" << VAR(param.keycode);
        return false;
    }

    return true;
}

MAA_CTRL_NS_END
