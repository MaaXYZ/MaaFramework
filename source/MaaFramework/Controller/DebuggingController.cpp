#include "DebuggingController.h"

#include "MaaFramework/MaaMsg.h"
#include "Utils/Logger.h"
#include "Utils/StringMisc.hpp"

#include <meojson/json.hpp>

MAA_CTRL_NS_BEGIN

DebuggingController::DebuggingController(std::string read_path, std::string write_path,
                                         std::shared_ptr<MAA_DBG_CTRL_UNIT_NS::ControllerAPI> controller,
                                         MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg)
    : ControllerMgr(callback, callback_arg), read_path_(std::move(read_path)), write_path_(std::move(write_path)),
      controller_(std::move(controller))
{
    LogDebug << VAR(read_path_) << VAR(write_path_) << VAR(controller_);
}

DebuggingController::~DebuggingController() {}

bool DebuggingController::_connect()
{
    LogFunc;

    if (!controller_) {
        LogError << "controller is nullptr" << VAR(controller_);
        return false;
    }

    if (!controller_->connect()) {
        LogError << "controller connect failed";
        return false;
    }

    return true;
}

std::string DebuggingController::_get_uuid() const
{
    if (!controller_) {
        LogError << "controller is nullptr" << VAR(controller_);
        return {};
    }

    return controller_->uuid();
}

std::pair<int, int> DebuggingController::_get_resolution() const
{
    if (!controller_) {
        LogError << "controller is nullptr" << VAR(controller_);
        return { 0, 0 };
    }

    auto [screen_width, screen_height] = controller_->resolution();
    return { screen_width, screen_height };
}

bool DebuggingController::_click(ClickParam param)
{
    if (!controller_) {
        LogError << "controller is nullptr" << VAR(controller_);
        return false;
    }

    if (!controller_->click(param.x, param.y)) {
        LogError << "controller click failed" << VAR(param.x) << VAR(param.y);
        return false;
    }

    return true;
}

bool DebuggingController::_swipe(SwipeParam param)
{
    if (!controller_) {
        LogError << "controller is nullptr" << VAR(controller_);
        return false;
    }

    if (!controller_->swipe(param.x1, param.y1, param.x2, param.y2, param.duration)) {
        LogError << "controller swipe failed" << VAR(param.x1) << VAR(param.y1) << VAR(param.x2) << VAR(param.y2)
                 << VAR(param.duration);
        return false;
    }

    return true;
}

bool DebuggingController::_touch_down(TouchParam param)
{
    if (!controller_) {
        LogError << "controller is nullptr" << VAR(controller_);
        return false;
    }

    if (!controller_->touch_down(param.contact, param.x, param.y, param.pressure)) {
        LogError << "controller touch_down failed" << VAR(param.contact) << VAR(param.x) << VAR(param.y)
                 << VAR(param.pressure);
        return false;
    }

    return true;
}

bool DebuggingController::_touch_move(TouchParam param)
{
    if (!controller_) {
        LogError << "controller is nullptr" << VAR(controller_);
        return false;
    }

    if (!controller_->touch_move(param.contact, param.x, param.y, param.pressure)) {
        LogError << "controller touch_move failed" << VAR(param.contact) << VAR(param.x) << VAR(param.y)
                 << VAR(param.pressure);
        return false;
    }

    return true;
}

bool DebuggingController::_touch_up(TouchParam param)
{
    if (!controller_) {
        LogError << "controller is nullptr" << VAR(controller_);
        return false;
    }

    if (!controller_->touch_up(param.contact)) {
        LogError << "controller touch_up failed" << VAR(param.contact);
        return false;
    }

    return true;
}

bool DebuggingController::_press_key(PressKeyParam param)
{
    if (!controller_) {
        LogError << "controller is nullptr" << VAR(controller_);
        return false;
    }

    if (!controller_->press_key(param.keycode)) {
        LogError << "controller press_key failed" << VAR(param.keycode);
        return false;
    }

    return true;
}

cv::Mat DebuggingController::_screencap()
{
    if (!controller_) {
        LogError << "controller is nullptr" << VAR(controller_);
        return {};
    }

    auto opt_mat = controller_->screencap();
    if (!opt_mat) {
        LogError << "controller screencap failed";
        return {};
    }

    if (opt_mat->empty()) {
        LogError << "controller screencap empty";
        return {};
    }

    return *opt_mat;
}

bool DebuggingController::_start_app(AppParam param)
{
    if (!controller_) {
        LogError << "controller is nullptr" << VAR(controller_);
        return false;
    }

    if (!controller_->start_app(param.package)) {
        LogError << "controller start failed" << VAR(param.package);
        return false;
    }

    return true;
}

bool DebuggingController::_stop_app(AppParam param)
{
    if (!controller_) {
        LogError << "controller is nullptr" << VAR(controller_);
        return false;
    }

    if (!controller_->stop_app(param.package)) {
        LogError << "controller stop failed" << VAR(param.package);
        return false;
    }

    return true;
}

MAA_CTRL_NS_END
