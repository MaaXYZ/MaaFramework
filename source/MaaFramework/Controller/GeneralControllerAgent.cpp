#include "GeneralControllerAgent.h"

#include "MaaFramework/MaaMsg.h"
#include "Utils/Logger.h"
#include "Utils/StringMisc.hpp"

#include <meojson/json.hpp>

MAA_CTRL_NS_BEGIN

GeneralControllerAgent::GeneralControllerAgent(
    std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> control_unit,
    MaaNotificationCallback notify,
    void* notify_trans_arg)
    : ControllerAgent(notify, notify_trans_arg)
    , control_unit_(std::move(control_unit))
{
}

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
        LogError << "controller swipe failed" << VAR(param.x1) << VAR(param.y1) << VAR(param.x2) << VAR(param.y2) << VAR(param.duration);
        return false;
    }

    return true;
}

bool GeneralControllerAgent::_multi_swipe(std::vector<SwipeParam> param)
{
    if (!control_unit_) {
        LogError << "controller is nullptr" << VAR(control_unit_);
        return false;
    }

    using ApiParam = MAA_CTRL_UNIT_NS::ControlUnitAPI::SwipeParam;
    std::vector<ApiParam> api_param;
    std::ranges::transform(param, std::back_inserter(api_param), [](const SwipeParam& swipe) {
        return ApiParam {
            .x1 = swipe.x1,
            .y1 = swipe.y1,
            .x2 = swipe.x2,
            .y2 = swipe.y2,
            .duration = swipe.duration,
            .starting = swipe.starting,
        };
    });

    if (!control_unit_->multi_swipe(api_param)) {
        LogError << "controller swipe failed" << VAR(json::array(param));
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
        LogError << "controller touch_down failed" << VAR(param.contact) << VAR(param.x) << VAR(param.y) << VAR(param.pressure);
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
        LogError << "controller touch_move failed" << VAR(param.contact) << VAR(param.x) << VAR(param.y) << VAR(param.pressure);
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

bool GeneralControllerAgent::_input_text(InputTextParam param)
{
    if (!control_unit_) {
        LogError << "controller is nullptr" << VAR(control_unit_);
        return false;
    }

    if (!control_unit_->input_text(param.text)) {
        LogError << "controller input_text failed" << VAR(param.text);
        return false;
    }

    return true;
}

MAA_CTRL_NS_END
