#include "ControllerMgr.h"

#include "MaaFramework/MaaMsg.h"
#include "Option/GlobalOptionMgr.h"
#include "Resource/ResourceMgr.h"
#include "Utils/ImageIo.h"
#include "Utils/NoWarningCV.hpp"

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
    case MaaCtrlOption_ScreenshotTargetLongSide:
        return set_image_target_long_side(value, val_size);
    case MaaCtrlOption_ScreenshotTargetShortSide:
        return set_image_target_short_side(value, val_size);

    case MaaCtrlOption_DefaultAppPackageEntry:
        return set_default_app_package_entry(value, val_size);
    case MaaCtrlOption_DefaultAppPackage:
        return set_default_app_package(value, val_size);

    case MaaCtrlOption_Recording:
        return set_recording(value, val_size);

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
    auto [xx, yy] = preproc_touch_point(x, y);
    ClickParam param { .x = xx, .y = yy };
    auto id = action_runner_->post({ .type = Action::Type::click, .param = std::move(param) });
    std::unique_lock lock { post_ids_mutex_ };
    post_ids_.emplace(id);
    return id;
}

MaaCtrlId ControllerMgr::post_swipe(int x1, int y1, int x2, int y2, int duration)
{
    auto [xx1, yy1] = preproc_touch_point(x1, y1);
    auto [xx2, yy2] = preproc_touch_point(x2, y2);
    SwipeParam param { .x1 = xx1, .y1 = yy1, .x2 = xx2, .y2 = yy2, .duration = duration };
    auto id = action_runner_->post({ .type = Action::Type::swipe, .param = std::move(param) });
    std::unique_lock lock { post_ids_mutex_ };
    post_ids_.emplace(id);
    return id;
}

MaaCtrlId ControllerMgr::post_press_key(int keycode)
{
    PressKeyParam param { .keycode = keycode };
    auto id = action_runner_->post({ .type = Action::Type::press_key, .param = std::move(param) });
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

MaaCtrlId ControllerMgr::post_touch_down(int contact, int x, int y, int pressure)
{
    auto [xx, yy] = preproc_touch_point(x, y);
    TouchParam param { .contact = contact, .x = xx, .y = yy, .pressure = pressure };
    auto id = action_runner_->post({ .type = Action::Type::touch_down, .param = std::move(param) });
    std::unique_lock lock { post_ids_mutex_ };
    post_ids_.emplace(id);
    return id;
}

MaaCtrlId ControllerMgr::post_touch_move(int contact, int x, int y, int pressure)
{
    auto [xx, yy] = preproc_touch_point(x, y);
    TouchParam param { .contact = contact, .x = xx, .y = yy, .pressure = pressure };
    auto id = action_runner_->post({ .type = Action::Type::touch_move, .param = std::move(param) });
    std::unique_lock lock { post_ids_mutex_ };
    post_ids_.emplace(id);
    return id;
}

MaaCtrlId ControllerMgr::post_touch_up(int contact)
{
    TouchParam param { .contact = contact };
    auto id = action_runner_->post({ .type = Action::Type::touch_up, .param = std::move(param) });
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

cv::Mat ControllerMgr::get_image() const
{
    return image_;
}

std::string ControllerMgr::get_uuid() const
{
    return _get_uuid();
}

void ControllerMgr::on_stop()
{
    action_runner_->release();
}

bool ControllerMgr::click(const cv::Rect& r)
{
    return click(rand_point(r));
}

bool ControllerMgr::click(const cv::Point& p)
{
    auto id = post_click(p.x, p.y);
    return status(id) == MaaStatus_Success;
}

bool ControllerMgr::swipe(const cv::Rect& r1, const cv::Rect& r2, int duration)
{
    return swipe(rand_point(r1), rand_point(r2), duration);
}

bool ControllerMgr::swipe(const cv::Point& p1, const cv::Point& p2, int duration)
{
    auto id = post_swipe(p1.x, p1.y, p2.x, p2.y, duration);
    return status(id) == MaaStatus_Success;
}

bool ControllerMgr::press_key(int keycode)
{
    auto id = post_press_key(keycode);
    return status(id) == MaaStatus_Success;
}

cv::Mat ControllerMgr::screencap()
{
    std::unique_lock<std::mutex> lock(image_mutex_);
    action_runner_->post({ .type = Action::Type::screencap }, true);
    return image_.clone();
}

bool ControllerMgr::start_app()
{
    if (default_app_package_entry_.empty()) {
        LogError << "default_app_package_entry_ is empty";
        return false;
    }
    return start_app(default_app_package_entry_);
}

bool ControllerMgr::stop_app()
{
    if (default_app_package_.empty()) {
        LogError << "default_app_package_ is empty";
        return false;
    }
    return stop_app(default_app_package_);
}

bool ControllerMgr::start_app(const std::string& package)
{
    auto id = action_runner_->post({ .type = Action::Type::start_app, .param = AppParam { .package = package } }, true);
    return status(id) == MaaStatus_Success;
}

bool ControllerMgr::stop_app(const std::string& package)
{
    auto id = action_runner_->post({ .type = Action::Type::stop_app, .param = AppParam { .package = package } }, true);
    return status(id) == MaaStatus_Success;
}

bool ControllerMgr::handle_connect()
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();

        init_recording();
    }

    connected_ = _connect();

    if (recording()) {
        auto [res_w, res_h] = _get_resolution();
        json::value info {
            { "type", "connect" },      { "success", connected_ },
            { "uuid", _get_uuid() },    { "resolution", { { "width", res_w }, { "height", res_h } } },
            { "version", MAA_VERSION },
        };
        append_recording(std::move(info), start_time, connected_);
    }

    return connected_;
}

bool ControllerMgr::handle_click(const ClickParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _click(param);

    if (recording()) {
        json::value info = {
            { "type", "click" },
            { "x", param.x },
            { "y", param.y },
        };
        append_recording(std::move(info), start_time, ret);
    }

    return ret;
}

bool ControllerMgr::handle_swipe(const SwipeParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _swipe(param);

    if (recording()) {
        json::value info = {
            { "type", "swipe" }, { "x1", param.x1 }, { "y1", param.y1 },
            { "x2", param.x2 },  { "y2", param.y2 }, { "duration", param.duration },
        };
        append_recording(std::move(info), start_time, ret);
    }

    return ret;
}

bool ControllerMgr::handle_touch_down(const TouchParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _touch_down(param);

    if (recording()) {
        json::value info = {
            { "type", "touch_down" }, { "contact", param.contact },   { "x", param.x },
            { "y", param.y },         { "pressure", param.pressure },
        };
        append_recording(std::move(info), start_time, ret);
    }

    return ret;
}

bool ControllerMgr::handle_touch_move(const TouchParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _touch_move(param);

    if (recording()) {
        json::value info = {
            { "type", "touch_move" }, { "contact", param.contact },   { "x", param.x },
            { "y", param.y },         { "pressure", param.pressure },
        };
        append_recording(std::move(info), start_time, ret);
    }

    return ret;
}

bool ControllerMgr::handle_touch_up(const TouchParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _touch_up(param);

    if (recording()) {
        json::value info = {
            { "type", "touch_up" },
            { "contact", param.contact },
        };
        append_recording(std::move(info), start_time, ret);
    }

    return ret;
}

bool ControllerMgr::handle_press_key(const PressKeyParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _press_key(param);

    if (recording()) {
        json::value info = {
            { "type", "press_key" },
            { "keycode", param.keycode },
        };
        append_recording(std::move(info), start_time, ret);
    }

    return ret;
}

bool ControllerMgr::handle_screencap()
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    cv::Mat raw_image = _screencap();
    bool ret = postproc_screenshot(raw_image);

    if (recording()) {
        auto image_relative_path = path("Screenshot") / path(now_filestem() + ".png");
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

bool ControllerMgr::handle_start_app(const AppParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _start_app(param);
    clear_target_image_size();

    if (recording()) {
        json::value info = {
            { "type", "start_app" },
            { "package", param.package },
        };
        append_recording(std::move(info), start_time, ret);
    }
    return ret;
}

bool ControllerMgr::handle_stop_app(const AppParam& param)
{
    std::chrono::steady_clock::time_point start_time;
    if (recording()) {
        start_time = std::chrono::steady_clock::now();
    }

    bool ret = _stop_app(param);
    clear_target_image_size();

    if (recording()) {
        json::value info = {
            { "type", "stop_app" },
            { "package", param.package },
        };
        append_recording(std::move(info), start_time, ret);
    }
    return ret;
}

bool ControllerMgr::recording() const
{
    return recording_ || GlobalOptionMgr::get_instance().recording();
}

void ControllerMgr::init_recording()
{
    auto recording_dir = GlobalOptionMgr::get_instance().logging_path() / "Recording";
    std::filesystem::create_directories(recording_dir);
    recording_path_ = recording_dir / MAA_FMT::format("MaaRecording_{}.txt", now_filestem());
}

void ControllerMgr::append_recording(json::value info, const std::chrono::steady_clock::time_point& start_time,
                                     bool success)
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
        ret = handle_connect();
        break;

    case Action::Type::click:
        ret = handle_click(std::get<ClickParam>(action.param));
        break;
    case Action::Type::swipe:
        ret = handle_swipe(std::get<SwipeParam>(action.param));
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
        notifier.notify(ret ? MaaMsg_Controller_Action_Completed : MaaMsg_Controller_Action_Failed, details);
    }

    return ret;
}

std::pair<int, int> ControllerMgr::preproc_touch_point(int x, int y)
{
    auto [res_w, res_h] = _get_resolution();

    if (image_target_width_ == 0 || image_target_height_ == 0) {
        // 正常来说连接完后都会截个图测试，那时候就会走到 check_and_calc_target_image_size，这里不应该是 0
        LogError << "Invalid image target size" << VAR(image_target_width_) << VAR(image_target_height_);
        return {};
    }

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

    if (!check_and_calc_target_image_size(raw)) {
        LogError << "Invalid target image size";
        return false;
    }

    cv::resize(raw, image_, { image_target_width_, image_target_height_ });
    return !image_.empty();
}

bool ControllerMgr::check_and_calc_target_image_size(const cv::Mat& raw)
{
    if (image_target_width_ != 0 && image_target_height_ != 0) {
        return true;
    }

    if (image_target_long_side_ == 0 && image_target_short_side_ == 0) {
        LogError << "Invalid image target size";
        return false;
    }

    int cur_width = raw.cols;
    int cur_height = raw.rows;

    LogDebug << "Re-calc image target size:" << VAR(image_target_long_side_) << VAR(image_target_short_side_)
             << VAR(cur_width) << VAR(cur_height);

    double scale = static_cast<double>(cur_width) / cur_height;

    if (image_target_short_side_ != 0) {
        if (cur_width > cur_height) {
            image_target_width_ = static_cast<int>(std::round(image_target_short_side_ * scale));
            image_target_height_ = image_target_short_side_;
        }
        else {
            image_target_width_ = image_target_short_side_;
            image_target_height_ = static_cast<int>(std::round(image_target_short_side_ / scale));
        }
    }
    else { // image_target_long_side_ != 0
        if (cur_width > cur_height) {
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

void ControllerMgr::clear_target_image_size()
{
    image_target_width_ = 0;
    image_target_height_ = 0;
}

bool ControllerMgr::set_image_target_long_side(MaaOptionValue value, MaaOptionValueSize val_size)
{
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

bool ControllerMgr::set_image_target_short_side(MaaOptionValue value, MaaOptionValueSize val_size)
{
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

bool ControllerMgr::set_recording(MaaOptionValue value, MaaOptionValueSize val_size)
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
