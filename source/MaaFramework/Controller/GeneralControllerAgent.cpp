#include "GeneralControllerAgent.h"

#include "MaaFramework/MaaMsg.h"
#include "Utils/Logger.h"
#include "Utils/StringMisc.hpp"

#include <meojson/json.hpp>

MAA_CTRL_NS_BEGIN

GeneralControllerAgent::GeneralControllerAgent(
    std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> control_unit,
    MaaControllerCallback callback,
    MaaCallbackTransparentArg callback_arg)
    : ControllerAgent(callback, callback_arg)
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

    bool micro = control_unit_->is_feature_supported(
        MAA_CTRL_UNIT_NS::ControlUnitAPI::Feature::MicroControl);

    bool ret = false;

    if (micro) {
        ret = micro_click(param.x, param.y);
    }
    else {
        ret = control_unit_->click(param.x, param.y);
    }

    if (!ret) {
        LogError << "controller click failed" << VAR(param.x) << VAR(param.y) << VAR(micro);
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

    bool micro = control_unit_->is_feature_supported(
        MAA_CTRL_UNIT_NS::ControlUnitAPI::Feature::MicroControl);

    bool ret = false;

    if (micro) {
        ret = micro_swipe(param.x1, param.y1, param.x2, param.y2, param.duration);
    }
    else {
        ret = control_unit_->swipe(param.x1, param.y1, param.x2, param.y2, param.duration);
    }

    if (!ret) {
        LogError << "controller swipe failed" << VAR(param.x1) << VAR(param.y1) << VAR(param.x2)
                 << VAR(param.y2) << VAR(param.duration) << VAR(micro);
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
        LogError << "controller touch_down failed" << VAR(param.contact) << VAR(param.x)
                 << VAR(param.y) << VAR(param.pressure);
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
        LogError << "controller touch_move failed" << VAR(param.contact) << VAR(param.x)
                 << VAR(param.y) << VAR(param.pressure);
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

bool GeneralControllerAgent::micro_click(int x, int y)
{
    if (!control_unit_) {
        return false;
    }

    constexpr int kContact = 0;
    constexpr int kPressure = 1;

    auto start = std::chrono::steady_clock::now();

    bool ret = control_unit_->touch_down(kContact, x, y, kPressure);

    std::this_thread::sleep_until(start + std::chrono::milliseconds(50));

    ret &= control_unit_->touch_up(kContact);

    return ret;
}

bool GeneralControllerAgent::micro_swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!control_unit_) {
        return false;
    }

    if (duration <= 0) {
        LogWarn << "duration out of range" << VAR(duration);
        duration = 500;
    }

    constexpr int kContact = 0;
    constexpr int kPressure = 1;

    auto start = std::chrono::steady_clock::now();
    auto now = start;

    bool ret = control_unit_->touch_down(kContact, x1, y1, kPressure);

    constexpr double kInterval = 10; // ms
    const double steps = duration / kInterval;
    const double x_step_len = (x2 - x1) / steps;
    const double y_step_len = (y2 - y1) / steps;
    const std::chrono::milliseconds delay(static_cast<int>(kInterval));

    for (int i = 0; i < steps; ++i) {
        int x = static_cast<int>(x1 + i * x_step_len);
        int y = static_cast<int>(y1 + i * y_step_len);

        std::this_thread::sleep_until(now + delay);

        now = std::chrono::steady_clock::now();
        ret &= control_unit_->touch_move(kContact, x, y, kPressure);
    }

    std::this_thread::sleep_until(now + delay);

    now = std::chrono::steady_clock::now();
    ret &= control_unit_->touch_move(kContact, x2, y2, kPressure);

    std::this_thread::sleep_until(now + delay);

    now = std::chrono::steady_clock::now();
    ret &= control_unit_->touch_up(kContact);

    return ret;
}

MAA_CTRL_NS_END
