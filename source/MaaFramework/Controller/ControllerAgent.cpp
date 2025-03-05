#include "ControllerAgent.h"

#include "Global/GlobalOptionMgr.h"
#include "MaaFramework/MaaMsg.h"
#include "Resource/ResourceMgr.h"
#include "Utils/ImageIo.h"
#include "Utils/NoWarningCV.hpp"

MAA_CTRL_NS_BEGIN

std::minstd_rand ControllerAgent::rand_engine_(std::random_device {}());

ControllerAgent::ControllerAgent(MaaNotificationCallback notify, void* notify_trans_arg)
    : notifier_(notify, notify_trans_arg)
{
    LogFunc << VAR_VOIDP(notify) << VAR_VOIDP(notify_trans_arg);

    action_runner_ =
        std::make_unique<AsyncRunner<Action>>(std::bind(&ControllerAgent::run_action, this, std::placeholders::_1, std::placeholders::_2));
}

ControllerAgent::~ControllerAgent()
{
    LogFunc;

    if (action_runner_) {
        action_runner_->wait_all();
    }
}

bool ControllerAgent::set_option(MaaCtrlOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogInfo << VAR(key) << VAR(value) << VAR(val_size);

    switch (key) {
    case MaaCtrlOption_ScreenshotTargetLongSide:
        return set_image_target_long_side(value, val_size);
    case MaaCtrlOption_ScreenshotTargetShortSide:
        return set_image_target_short_side(value, val_size);
    case MaaCtrlOption_ScreenshotUseRawSize:
        return set_image_use_raw_size(value, val_size);
    case MaaCtrlOption_Recording:
        return set_recording(value, val_size);

    default:
        LogError << "Unknown key" << VAR(key) << VAR(value);
        return false;
    }
}

MaaCtrlId ControllerAgent::post_connection()
{
    auto id = post_connection_impl();
    focus_id(id);
    return id;
}

MaaCtrlId ControllerAgent::post_click(int x, int y)
{
    auto id = post_click_impl(x, y);
    focus_id(id);
    return id;
}

MaaCtrlId ControllerAgent::post_swipe(int x1, int y1, int x2, int y2, int duration)
{
    auto id = post_swipe_impl(x1, y1, x2, y2, duration);
    focus_id(id);
    return id;
}

MaaCtrlId ControllerAgent::post_press_key(int keycode)
{
    auto id = post_press_key_impl(keycode);
    focus_id(id);
    return id;
}

MaaCtrlId ControllerAgent::post_input_text(const std::string& text)
{
    auto id = post_input_text_impl(text);
    focus_id(id);
    return id;
}

MaaCtrlId ControllerAgent::post_start_app(const std::string& intent)
{
    auto id = post_start_app_impl(intent);
    focus_id(id);
    return id;
}

MaaCtrlId ControllerAgent::post_stop_app(const std::string& intent)
{
    auto id = post_stop_app_impl(intent);
    focus_id(id);
    return id;
}

MaaCtrlId ControllerAgent::post_screencap()
{
    auto id = post_screencap_impl();
    focus_id(id);
    return id;
}

MaaCtrlId ControllerAgent::post_touch_down(int contact, int x, int y, int pressure)
{
    auto id = post_touch_down_impl(contact, x, y, pressure);
    focus_id(id);
    return id;
}

MaaCtrlId ControllerAgent::post_touch_move(int contact, int x, int y, int pressure)
{
    auto id = post_touch_move_impl(contact, x, y, pressure);
    focus_id(id);
    return id;
}

MaaCtrlId ControllerAgent::post_touch_up(int contact)
{
    auto id = post_touch_up_impl(contact);
    focus_id(id);
    return id;
}

MaaStatus ControllerAgent::status(MaaCtrlId ctrl_id) const
{
    if (!action_runner_) {
        LogError << "action_runner_ is nullptr";
        return MaaStatus_Invalid;
    }
    return action_runner_->status(ctrl_id);
}

MaaStatus ControllerAgent::wait(MaaCtrlId ctrl_id) const
{
    if (!action_runner_) {
        LogError << "action_runner_ is nullptr";
        return MaaStatus_Invalid;
    }
    if (ctrl_id == MaaInvalidId) {
        return MaaStatus_Invalid;
    }

    action_runner_->wait(ctrl_id);
    return action_runner_->status(ctrl_id);
}

bool ControllerAgent::connected() const
{
    return connected_;
}

cv::Mat ControllerAgent::cached_image() const
{
    return image_;
}

std::string ControllerAgent::get_uuid()
{
    if (uuid_cache_.empty()) {
        request_uuid();
    }
    return uuid_cache_;
}

void ControllerAgent::post_stop()
{
    LogFunc;

    need_to_stop_ = true;

    if (action_runner_ && action_runner_->running()) {
        action_runner_->clear();
    }
}

bool ControllerAgent::running() const
{
    return action_runner_ && action_runner_->running();
}

bool ControllerAgent::click(const cv::Rect& r)
{
    return click(rand_point(r));
}

bool ControllerAgent::click(const cv::Point& p)
{
    auto id = post_click_impl(p.x, p.y);
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::swipe(const cv::Rect& r1, const cv::Rect& r2, int duration)
{
    return swipe(rand_point(r1), rand_point(r2), duration);
}

bool ControllerAgent::swipe(const cv::Point& p1, const cv::Point& p2, int duration)
{
    auto id = post_swipe_impl(p1.x, p1.y, p2.x, p2.y, duration);
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::multi_swipe(const std::vector<SwipeParamWithRect>& swipes)
{
    std::vector<SwipeParam> dst_vec;
    for (const auto& src : swipes) {
        auto p1 = rand_point(src.r1);
        auto p2 = rand_point(src.r2);
        dst_vec.emplace_back(SwipeParam {
            .x1 = p1.x,
            .y1 = p1.y,
            .x2 = p2.x,
            .y2 = p2.y,
            .duration = static_cast<int>(src.duration),
            .starting = static_cast<int>(src.starting),
        });
    }

    auto id = post_multi_swipe_impl(dst_vec);
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::press_key(int keycode)
{
    auto id = post_press_key_impl(keycode);
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::input_text(const std::string& text)
{
    auto id = post_input_text_impl(text);
    return wait(id) == MaaStatus_Succeeded;
}

cv::Mat ControllerAgent::screencap()
{
    std::unique_lock lock(image_mutex_);
    auto id = post_screencap_impl();
    if (wait(id) != MaaStatus_Succeeded) {
        return {};
    }
    return image_.clone();
}

bool ControllerAgent::start_app(const std::string& package)
{
    auto id = post({ .type = Action::Type::start_app, .param = AppParam { .package = package } });
    return wait(id) == MaaStatus_Succeeded;
}

bool ControllerAgent::stop_app(const std::string& package)
{
    auto id = post({ .type = Action::Type::stop_app, .param = AppParam { .package = package } });
    return wait(id) == MaaStatus_Succeeded;
}

MaaCtrlId ControllerAgent::post(Action action)
{
    if (!check_stop()) {
        return MaaInvalidId;
    }

    if (!action_runner_) {
        return MaaInvalidId;
    }
    return action_runner_->post(std::move(action));
}

void ControllerAgent::focus_id(MaaCtrlId id)
{
    if (id == MaaInvalidId) {
        return;
    }

    std::unique_lock lock { focus_ids_mutex_ };
    focus_ids_.emplace(id);
}

MaaCtrlId ControllerAgent::post_connection_impl()
{
    return post({ .type = Action::Type::connect });
}

MaaCtrlId ControllerAgent::post_click_impl(int x, int y)
{
    auto [xx, yy] = preproc_touch_point(x, y);
    ClickParam param { .x = xx, .y = yy };
    return post({ .type = Action::Type::click, .param = std::move(param) });
}

MaaCtrlId ControllerAgent::post_swipe_impl(int x1, int y1, int x2, int y2, int duration)
{
    auto [xx1, yy1] = preproc_touch_point(x1, y1);
    auto [xx2, yy2] = preproc_touch_point(x2, y2);
    SwipeParam param { .x1 = xx1, .y1 = yy1, .x2 = xx2, .y2 = yy2, .duration = duration };
    return post({ .type = Action::Type::swipe, .param = std::move(param) });
}

MaaCtrlId ControllerAgent::post_multi_swipe_impl(const std::vector<SwipeParam>& swipes)
{
    std::vector<SwipeParam> dst = swipes;
    for (SwipeParam& d : dst) {
        std::tie(d.x1, d.y1) = preproc_touch_point(d.x1, d.y1);
        std::tie(d.x2, d.y2) = preproc_touch_point(d.x2, d.y2);
    }

    return post({ .type = Action::Type::multi_swipe, .param = std::move(dst) });
}

MaaCtrlId ControllerAgent::post_press_key_impl(int keycode)
{
    PressKeyParam param { .keycode = keycode };
    return post({ .type = Action::Type::press_key, .param = std::move(param) });
}

MaaCtrlId ControllerAgent::post_input_text_impl(const std::string& text)
{
    InputTextParam param { .text = text };
    return post({ .type = Action::Type::input_text, .param = std::move(param) });
}

MaaCtrlId ControllerAgent::post_start_app_impl(const std::string& intent)
{
    AppParam param { .package = intent };
    return post({ .type = Action::Type::start_app, .param = std::move(param) });
}

MaaCtrlId ControllerAgent::post_stop_app_impl(const std::string& intent)
{
    AppParam param { .package = intent };
    return post({ .type = Action::Type::stop_app, .param = std::move(param) });
}

MaaCtrlId ControllerAgent::post_screencap_impl()
{
    return post({ .type = Action::Type::screencap });
}

MaaCtrlId ControllerAgent::post_touch_down_impl(int contact, int x, int y, int pressure)
{
    auto [xx, yy] = preproc_touch_point(x, y);
    TouchParam param { .contact = contact, .x = xx, .y = yy, .pressure = pressure };
    return post({ .type = Action::Type::touch_down, .param = std::move(param) });
}

MaaCtrlId ControllerAgent::post_touch_move_impl(int contact, int x, int y, int pressure)
{
    auto [xx, yy] = preproc_touch_point(x, y);
    TouchParam param { .contact = contact, .x = xx, .y = yy, .pressure = pressure };
    return post({ .type = Action::Type::touch_move, .param = std::move(param) });
}

MaaCtrlId ControllerAgent::post_touch_up_impl(int contact)
{
    TouchParam param { .contact = contact };
    return post({ .type = Action::Type::touch_up, .param = std::move(param) });
}

bool ControllerAgent::handle_connect()
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();

        init_recording();
    }

    connected_ = _connect();

    request_uuid();

    if (recording()) {
        json::value info {
            { "type", "connect" },
            { "uuid", get_uuid() },
            { "version", MAA_VERSION },
        };
        append_recording(std::move(info), start_time, connected_);
    }

    return connected_;
}

bool ControllerAgent::handle_click(const ClickParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _click(param);

    if (recording()) {
        json::value info = param;
        info |= { { "type", "click" } };
        append_recording(std::move(info), start_time, ret);
    }

    return ret;
}

bool ControllerAgent::handle_swipe(const SwipeParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _swipe(param);

    if (recording()) {
        json::value info = param;
        info |= { { "type", "swipe" } };
        append_recording(std::move(info), start_time, ret);
    }

    return ret;
}

bool ControllerAgent::handle_multi_swipe(const std::vector<SwipeParam>& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _multi_swipe(param);

    if (recording()) {
        json::value info = { { "type", "multi_swipe" }, { "swipes", json::array(param) } };
        append_recording(std::move(info), start_time, ret);
    }

    return ret;
}

bool ControllerAgent::handle_touch_down(const TouchParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _touch_down(param);

    if (recording()) {
        json::value info = param;
        info |= { { "type", "touch_down" } };
        append_recording(std::move(info), start_time, ret);
    }

    return ret;
}

bool ControllerAgent::handle_touch_move(const TouchParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _touch_move(param);

    if (recording()) {
        json::value info = param;
        info |= { { "type", "touch_move" } };
        append_recording(std::move(info), start_time, ret);
    }

    return ret;
}

bool ControllerAgent::handle_touch_up(const TouchParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _touch_up(param);

    if (recording()) {
        json::value info = param;
        info |= { { "type", "touch_up" } };
        append_recording(std::move(info), start_time, ret);
    }

    return ret;
}

bool ControllerAgent::handle_press_key(const PressKeyParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _press_key(param);

    if (recording()) {
        json::value info = param;
        info |= { { "type", "press_key" } };
        append_recording(std::move(info), start_time, ret);
    }

    return ret;
}

bool ControllerAgent::handle_input_text(const InputTextParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _input_text(param);

    if (recording()) {
        json::value info = param;
        info |= { { "type", "input_text" } };
        append_recording(std::move(info), start_time, ret);
    }

    return ret;
}

bool ControllerAgent::handle_screencap()
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    auto opt = _screencap();
    if (!opt) {
        LogError << "controller screencap failed";
        return false;
    }

    cv::Mat raw_image = std::move(*opt);
    bool ret = postproc_screenshot(raw_image);

    if (recording()) {
        auto image_relative_path = path("screenshot") / path(format_now_for_filename() + ".png");
        auto image_path = recording_path_.parent_path() / image_relative_path;
        MAA_NS::imwrite(image_path, raw_image);

        json::value info = {
            { "type", "screencap" },
            { "path", path_to_utf8_string(image_relative_path) },
        };
        append_recording(std::move(info), start_time, ret);
    }

    return ret;
}

bool ControllerAgent::handle_start_app(const AppParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _start_app(param);

    if (recording()) {
        json::value info = param;
        info |= { { "type", "start_app" } };
        append_recording(std::move(info), start_time, ret);
    }
    return ret;
}

bool ControllerAgent::handle_stop_app(const AppParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _stop_app(param);

    if (recording()) {
        json::value info = param;
        info |= { { "type", "stop_app" } };
        append_recording(std::move(info), start_time, ret);
    }
    return ret;
}

bool ControllerAgent::recording() const
{
    return recording_ || GlobalOptionMgr::get_instance().recording();
}

void ControllerAgent::init_recording()
{
    auto recording_dir = GlobalOptionMgr::get_instance().log_dir() / "recording";
    std::filesystem::create_directories(recording_dir);
    recording_path_ = recording_dir / std::format("maa_recording_{}.txt", format_now_for_filename());
}

void ControllerAgent::append_recording(json::value info, const std::chrono::steady_clock::time_point& start_time, bool success)
{
    if (!recording()) {
        return;
    }

    info["time"] = start_time.time_since_epoch().count();
    info["cost"] = duration_since(start_time).count();
    info["success"] = success;

    std::ofstream ofs(recording_path_, std::ios::app);
    ofs << info.to_string() << "\n";
    ofs.close();
}

bool ControllerAgent::check_stop()
{
    if (!need_to_stop_) {
        return true;
    }

    if (running()) {
        LogError << "stopping, ignore new post";
        return false;
    }

    need_to_stop_ = false;
    return true;
}

cv::Point ControllerAgent::rand_point(const cv::Rect& r)
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

bool ControllerAgent::run_action(typename AsyncRunner<Action>::Id id, Action action)
{
    // LogFunc << VAR(id) << VAR(action);

    bool ret = false;

    bool notify = false;
    {
        std::unique_lock lock { focus_ids_mutex_ };
        notify = focus_ids_.erase(id) > 0;
    }

    std::stringstream ss;
    ss << action;
    const json::value cb_detail = {
        { "ctrl_id", id },
        { "uuid", get_uuid() },
        { "action", std::move(ss).str() },
    };
    if (notify) {
        notifier_.notify(MaaMsg_Controller_Action_Starting, cb_detail);
    }

    switch (action.type) {
    case Action::Type::connect:
        ret = handle_connect();
        break;

    case Action::Type::click:
        ret = handle_click(std::get<ClickParam>(action.param));
        break;
    case Action::Type::swipe:
        ret = handle_swipe(std::get<SwipeParam>(action.param));
        break;
    case Action::Type::multi_swipe:
        ret = handle_multi_swipe(std::get<std::vector<SwipeParam>>(action.param));
        break;

    case Action::Type::touch_down:
        ret = handle_touch_down(std::get<TouchParam>(action.param));
        break;
    case Action::Type::touch_move:
        ret = handle_touch_move(std::get<TouchParam>(action.param));
        break;
    case Action::Type::touch_up:
        ret = handle_touch_up(std::get<TouchParam>(action.param));
        break;

    case Action::Type::press_key:
        ret = handle_press_key(std::get<PressKeyParam>(action.param));
        break;
    case Action::Type::input_text:
        ret = handle_input_text(std::get<InputTextParam>(action.param));
        break;

    case Action::Type::screencap:
        ret = handle_screencap();
        break;

    case Action::Type::start_app:
        ret = handle_start_app(std::get<AppParam>(action.param));
        break;
    case Action::Type::stop_app:
        ret = handle_stop_app(std::get<AppParam>(action.param));
        break;

    default:
        LogError << "Unknown action type" << VAR(static_cast<int>(action.type));
        ret = false;
    }

    if (notify) {
        notifier_.notify(ret ? MaaMsg_Controller_Action_Succeeded : MaaMsg_Controller_Action_Failed, cb_detail);
    }

    return ret;
}

std::pair<int, int> ControllerAgent::preproc_touch_point(int x, int y)
{
    if (image_target_width_ == 0 || image_target_height_ == 0) {
        LogWarn << "Invalid image target size" << VAR(image_target_width_) << VAR(image_target_height_);

        if (!init_scale_info()) {
            return {};
        }
    }

    double scale_width = static_cast<double>(image_raw_width_) / image_target_width_;
    double scale_height = static_cast<double>(image_raw_height_) / image_target_height_;

    int proced_x = static_cast<int>(std::round(x * scale_width));
    int proced_y = static_cast<int>(std::round(y * scale_height));

    return { proced_x, proced_y };
}

bool ControllerAgent::postproc_screenshot(const cv::Mat& raw)
{
    if (raw.empty()) {
        image_ = cv::Mat();
        LogError << "Empty screenshot";
        return false;
    }

    if (raw.cols != image_raw_width_ || raw.rows != image_raw_height_ || image_target_width_ == 0 || image_target_height_ == 0) {
        LogInfo << "Resolution changed" << VAR(raw.cols) << VAR(raw.rows) << VAR(image_raw_width_) << VAR(image_raw_height_);

        image_raw_width_ = raw.cols;
        image_raw_height_ = raw.rows;

        if (!calc_target_image_size()) {
            image_ = cv::Mat();
            LogError << "Invalid target image size";
            return false;
        }
    }

    cv::resize(raw, image_, { image_target_width_, image_target_height_ });
    return !image_.empty();
}

bool ControllerAgent::calc_target_image_size()
{
    if (image_target_long_side_ == 0 && image_target_short_side_ == 0) {
        LogError << "Invalid image target size";
        return false;
    }

    if (image_use_raw_size_) {
        LogDebug << "image_use_raw_size_" << VAR(image_raw_width_) << VAR(image_raw_height_);
        image_target_width_ = image_raw_width_;
        image_target_height_ = image_raw_height_;
        return true;
    }

    LogDebug << "Re-calc image target size:" << VAR(image_target_long_side_) << VAR(image_target_short_side_) << VAR(image_raw_width_)
             << VAR(image_raw_height_);

    double scale = static_cast<double>(image_raw_width_) / image_raw_height_;

    if (image_target_short_side_ != 0) {
        if (image_raw_width_ > image_raw_height_) {
            image_target_width_ = static_cast<int>(std::round(image_target_short_side_ * scale));
            image_target_height_ = image_target_short_side_;
        }
        else {
            image_target_width_ = image_target_short_side_;
            image_target_height_ = static_cast<int>(std::round(image_target_short_side_ / scale));
        }
    }
    else { // image_target_long_side_ != 0
        if (image_raw_width_ > image_raw_height_) {
            image_target_width_ = image_target_long_side_;
            image_target_height_ = static_cast<int>(std::round(image_target_long_side_ / scale));
        }
        else {
            image_target_width_ = static_cast<int>(std::round(image_target_long_side_ * scale));
            image_target_height_ = image_target_long_side_;
        }
    }

    LogInfo << VAR(image_target_width_) << VAR(image_target_height_);
    return true;
}

void ControllerAgent::clear_target_image_size()
{
    LogDebug;

    image_target_width_ = 0;
    image_target_height_ = 0;
}

bool ControllerAgent::request_uuid()
{
    uuid_cache_.clear();

    auto uuid_opt = _request_uuid();
    if (!uuid_opt) {
        LogError << "controller request uuid failed";
        return false;
    }
    uuid_cache_ = *uuid_opt;
    return true;
}

bool ControllerAgent::init_scale_info()
{
    // 实际是通过 postproc_screenshot 初始化的
    return !screencap().empty();
}

bool ControllerAgent::set_image_target_long_side(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogDebug;

    if (val_size != sizeof(image_target_long_side_)) {
        LogError << "invalid value size: " << val_size;
        return false;
    }
    image_target_long_side_ = *reinterpret_cast<int*>(value);
    image_target_short_side_ = 0;

    clear_target_image_size();

    LogInfo << "image_target_width_ = " << image_target_long_side_;
    return true;
}

bool ControllerAgent::set_image_target_short_side(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogDebug;

    if (val_size != sizeof(image_target_short_side_)) {
        LogError << "invalid value size: " << val_size;
        return false;
    }
    image_target_long_side_ = 0;
    image_target_short_side_ = *reinterpret_cast<int*>(value);

    clear_target_image_size();

    LogInfo << "image_target_height_ = " << image_target_short_side_;
    return true;
}

bool ControllerAgent::set_image_use_raw_size(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogDebug;

    if (val_size != sizeof(bool)) {
        LogError << "invalid value size: " << val_size;
        return false;
    }
    image_use_raw_size_ = *reinterpret_cast<bool*>(value);

    clear_target_image_size();

    return true;
}

bool ControllerAgent::set_recording(MaaOptionValue value, MaaOptionValueSize val_size)
{
    if (val_size != sizeof(recording_)) {
        LogError << "invalid value size: " << val_size;
        return false;
    }
    recording_ = *reinterpret_cast<bool*>(value);

    return true;
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
    case Action::Type::touch_down:
        os << "touch_down";
        break;
    case Action::Type::touch_move:
        os << "touch_move";
        break;
    case Action::Type::touch_up:
        os << "touch_up";
        break;
    case Action::Type::press_key:
        os << "press_key";
        break;
    case Action::Type::input_text:
        os << "input_text";
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
