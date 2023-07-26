#include "ControllerMgr.h"

#include "Resource/ResourceMgr.h"
#include "Utils/Math.hpp"
#include "Utils/NoWarningCV.h"
#include <MaaMsg.h>

#include <tuple>

MAA_CTRL_NS_BEGIN

std::minstd_rand ControllerMgr::rand_engine_(std::random_device {}());

ControllerMgr::ControllerMgr(MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg)
    : notifier(callback, callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    action_runner_ = std::make_unique<AsyncRunner<Action>>(
        std::bind(&ControllerMgr::run_action, this, std::placeholders::_1, std::placeholders::_2));
}

ControllerMgr::~ControllerMgr()
{
    LogFunc;

    if (action_runner_) {
        action_runner_->release();
    }
}

bool ControllerMgr::set_option(MaaCtrlOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogInfo << VAR(key) << VAR(value) << VAR(val_size);

    switch (key) {
    case MaaCtrlOption_ScreenshotTargetWidth:
        return set_target_width(value, val_size);
    case MaaCtrlOption_ScreenshotTargetHeight:
        return set_target_height(value, val_size);

    case MaaCtrlOption_DefaultAppPackageEntry:
        return set_default_app_package_entry(value, val_size);
    case MaaCtrlOption_DefaultAppPackage:
        return set_default_app_package(value, val_size);

    default:
        LogError << "Unknown key" << VAR(key) << VAR(value);
        return false;
    }
}

MaaCtrlId ControllerMgr::post_connection()
{
    auto id = action_runner_->post({ .type = Action::Type::connect });
    std::unique_lock lock { post_ids_mutex_ };
    post_ids_.emplace(id);
    return id;
}

MaaCtrlId ControllerMgr::post_click(int x, int y)
{
    preproce_touch_coord(x, y);
    ClickParam param { .x = x, .y = y };
    auto id = action_runner_->post({ .type = Action::Type::click, .param = std::move(param) });
    std::unique_lock lock { post_ids_mutex_ };
    post_ids_.emplace(id);
    return id;
}

MaaCtrlId ControllerMgr::post_swipe(std::vector<int> x_steps, std::vector<int> y_steps, std::vector<int> step_delay)
{
    SwipeParam param;
    for (size_t i = 0; i != x_steps.size(); ++i) {
        SwipeParam::Step step {
            .x = x_steps[i],
            .y = y_steps[i],
            .delay = step_delay[i],
        };
        param.steps.emplace_back(std::move(step));
    }

    auto id = action_runner_->post({ .type = Action::Type::swipe, .param = std::move(param) });
    std::unique_lock lock { post_ids_mutex_ };
    post_ids_.emplace(id);
    return id;
}

MaaCtrlId ControllerMgr::post_screencap()
{
    auto id = action_runner_->post({ .type = Action::Type::screencap });
    std::unique_lock lock { post_ids_mutex_ };
    post_ids_.emplace(id);
    return id;
}

MaaStatus ControllerMgr::status(MaaCtrlId ctrl_id) const
{
    if (!action_runner_) {
        LogError << "action_runner_ is nullptr";
        return MaaStatus_Invalid;
    }
    return action_runner_->status(ctrl_id);
}

MaaStatus ControllerMgr::wait(MaaCtrlId ctrl_id) const
{
    if (!action_runner_) {
        LogError << "action_runner_ is nullptr";
        return MaaStatus_Invalid;
    }
    action_runner_->wait(ctrl_id);
    return action_runner_->status(ctrl_id);
}

MaaBool ControllerMgr::connected() const
{
    return connected_;
}

std::vector<uint8_t> ControllerMgr::get_image_cache() const
{
    std::vector<uint8_t> buff;
    cv::imencode(".png", image_, buff);
    return buff;
}

void ControllerMgr::on_stop()
{
    action_runner_->release();
}

void ControllerMgr::click(const cv::Rect& r)
{
    click(rand_point(r));
}

void ControllerMgr::click(const cv::Point& p)
{
    auto [x, y] = preproce_touch_coord(p.x, p.y);
    ClickParam param { .x = x, .y = y };
    action_runner_->post({ .type = Action::Type::click, .param = std::move(param) }, true);
}

void ControllerMgr::swipe(const cv::Rect& r1, const cv::Rect& r2, int duration)
{
    swipe(rand_point(r1), rand_point(r2), duration);
}

void ControllerMgr::swipe(const cv::Point& p1, const cv::Point& p2, int duration)
{
    constexpr int SampleDelay = 2;

    auto [px1, py1] = preproce_touch_coord(p1.x, p1.y);
    auto [px2, py2] = preproce_touch_coord(p2.x, p2.y);

    SwipeParam param;
    auto cs = CubicSpline::smooth_in_out(1, 1);
    for (int i = 0; i < duration; i += SampleDelay) {
        auto progress = cs(static_cast<double>(i) / duration);
        int x = static_cast<int>(round(std::lerp(px1, px2, progress)));
        int y = static_cast<int>(round(std::lerp(py1, py2, progress)));
        param.steps.emplace_back(SwipeParam::Step { .x = x, .y = y, .delay = SampleDelay });
    }
    action_runner_->post({ .type = Action::Type::swipe, .param = std::move(param) }, true);
}

void ControllerMgr::press_key(int keycode)
{
    action_runner_->post({ .type = Action::Type::press_key, .param = PressKeyParam { .keycode = keycode } }, true);
}

cv::Mat ControllerMgr::screencap()
{
    std::unique_lock<std::mutex> lock(image_mutex_);
    action_runner_->post({ .type = Action::Type::screencap }, true);
    return image_.clone();
}

void ControllerMgr::start_app()
{
    if (default_app_package_entry_.empty()) {
        LogError << "default_app_package_entry_ is empty";
        return;
    }
    start_app(default_app_package_entry_);
}

void ControllerMgr::stop_app()
{
    if (default_app_package_.empty()) {
        LogError << "default_app_package_ is empty";
        return;
    }
    stop_app(default_app_package_);
}

void ControllerMgr::start_app(const std::string& package)
{
    action_runner_->post({ .type = Action::Type::start_app, .param = AppParam { .package = package } }, true);
}

void ControllerMgr::stop_app(const std::string& package)
{
    action_runner_->post({ .type = Action::Type::stop_app, .param = AppParam { .package = package } }, true);
}

cv::Point ControllerMgr::rand_point(const cv::Rect& r)
{
    int x = 0, y = 0;

    if (r.width == 0) {
        x = r.x;
    }
    else {
        int x_rand = std::poisson_distribution<int>(r.width / 2.)(rand_engine_);
        x = x_rand + r.x;
    }

    if (r.height == 0) {
        y = r.y;
    }
    else {
        int y_rand = std::poisson_distribution<int>(r.height / 2.)(rand_engine_);
        y = y_rand + r.y;
    }

    return { x, y };
}

bool ControllerMgr::run_action(typename AsyncRunner<Action>::Id id, Action action)
{
    // LogFunc << VAR(id) << VAR(action);

    bool ret = false;

    bool notify = false;
    {
        std::unique_lock lock { post_ids_mutex_ };
        notify = post_ids_.erase(id) > 0;
    }

    const json::value details = {
        { "id", id },
        { "uuid", get_uuid() },
    };
    if (notify) {
        notifier.notify(MaaMsg_Controller_Action_Started, details);
    }

    switch (action.type) {
    case Action::Type::connect:
        connected_ = _connect();
        ret = connected_;
        break;

    case Action::Type::click:
        _click(std::get<ClickParam>(action.param));
        ret = true;
        break;
    case Action::Type::swipe:
        _swipe(std::get<SwipeParam>(action.param));
        ret = true;
        break;
    case Action::Type::press_key:
        _press_key(std::get<PressKeyParam>(action.param));
        ret = true;
        break;

    case Action::Type::screencap:
        ret = postproc_screenshot(_screencap());
        break;

    case Action::Type::start_app:
        ret = _start_app(std::get<AppParam>(action.param));
        break;
    case Action::Type::stop_app:
        ret = _stop_app(std::get<AppParam>(action.param));
        break;

    default:
        LogError << "Unknown action type" << VAR(static_cast<int>(action.type));
        ret = false;
    }

    if (notify) {
        notifier.notify(ret ? MaaMsg_Controller_Action_Completed : MaaMsg_Controller_Action_Failed, details);
    }

    return ret;
}

std::pair<int, int> ControllerMgr::preproce_touch_coord(int x, int y)
{
    auto [res_w, res_h] = _get_resolution();

    double scale_width = static_cast<double>(res_w) / image_target_width_;
    double scale_height = static_cast<double>(res_h) / image_target_height_;

    int proced_x = static_cast<int>(std::round(x * scale_width));
    int proced_y = static_cast<int>(std::round(y * scale_height));

    return { proced_x, proced_y };
}

bool ControllerMgr::postproc_screenshot(const cv::Mat& raw)
{
    if (raw.empty()) {
        LogError << "Empty screenshot";
        return false;
    }

    auto [res_w, res_h] = _get_resolution();
    if (raw.cols != res_w || raw.rows != res_h) {
        LogWarn << "Invalid resolution" << VAR(raw.cols) << VAR(raw.rows) << VAR(res_w) << VAR(res_h);
    }

    if (image_target_width_ == 0) {
        double scale = static_cast<double>(raw.cols) / raw.rows;
        image_target_width_ = static_cast<int>(std::round(image_target_height_ * scale));
        LogInfo << VAR(image_target_width_) << VAR(image_target_height_);
    }
    if (image_target_height_ == 0) {
        double scale = static_cast<double>(raw.rows) / raw.cols;
        image_target_height_ = static_cast<int>(std::round(image_target_width_ * scale));
        LogInfo << VAR(image_target_width_) << VAR(image_target_height_);
    }

    if (raw.cols == image_target_width_ && raw.rows == image_target_height_) {
        image_ = raw;
        return true;
    }

    cv::resize(raw, image_, { image_target_width_, image_target_height_ });
    return !image_.empty();
}

bool ControllerMgr::set_target_width(MaaOptionValue value, MaaOptionValueSize val_size)
{
    if (val_size != sizeof(image_target_width_)) {
        LogError << "invalid value size: " << val_size;
        return false;
    }
    image_target_width_ = *reinterpret_cast<int*>(value);
    image_target_height_ = 0;

    LogInfo << "image_target_width_ = " << image_target_width_;
    return true;
}

bool ControllerMgr::set_target_height(MaaOptionValue value, MaaOptionValueSize val_size)
{
    if (val_size != sizeof(image_target_height_)) {
        LogError << "invalid value size: " << val_size;
        return false;
    }
    image_target_width_ = 0;
    image_target_height_ = *reinterpret_cast<int*>(value);

    LogInfo << "image_target_height_ = " << image_target_height_;
    return true;
}

bool ControllerMgr::set_default_app_package_entry(MaaOptionValue value, MaaOptionValueSize val_size)
{
    std::string_view package(reinterpret_cast<char*>(value), val_size);
    default_app_package_entry_ = package;
    return true;
}

bool ControllerMgr::set_default_app_package(MaaOptionValue value, MaaOptionValueSize val_size)
{
    std::string_view package(reinterpret_cast<char*>(value), val_size);
    default_app_package_ = package;
    return true;
}

std::ostream& operator<<(std::ostream& os, const SwipeParam::Step& step)
{
    os << VAR_RAW(step.x) << VAR_RAW(step.y) << VAR_RAW(step.delay);
    return os;
}

std::ostream& operator<<(std::ostream& os, const Action& action)
{
    switch (action.type) {
    case Action::Type::connect:
        os << "connect";
        break;
    case Action::Type::click:
        os << "click";
        break;
    case Action::Type::swipe:
        os << "swipe";
        break;
    case Action::Type::screencap:
        os << "screencap";
        break;
    case Action::Type::start_app:
        os << "start_app";
        break;
    case Action::Type::stop_app:
        os << "stop_app";
        break;
    default:
        os << "unknown action";
        break;
    }
    return os;
}

MAA_CTRL_NS_END
