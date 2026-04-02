#include "AndroidNativeControlUnitMgr.h"

#include <algorithm>
#include <utility>

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

using namespace AndroidNativeNS;

namespace
{
constexpr auto kUuid = "AndroidNativeController";
} // namespace

AndroidNativeControlUnitMgr::AndroidNativeControlUnitMgr(AndroidNativeControlUnitConfig config)
    : config_(std::move(config))
{
    LogFunc << VAR(config_.library_path) << VAR(config_.touch_width) << VAR(config_.touch_height) << VAR(config_.display_id)
            << VAR(config_.force_stop);
}

AndroidNativeControlUnitMgr::~AndroidNativeControlUnitMgr()
{
    if (funcs_) {
        AndroidNativeExternalLibraryHolder::unload_library();
        funcs_.reset();
    }
}

bool AndroidNativeControlUnitMgr::connect()
{
    LogFunc;

    if (connected()) {
        return true;
    }

    auto funcs_opt = AndroidNativeExternalLibraryHolder::create_functions(config_.library_path);
    if (!funcs_opt) {
        LogError << "Failed to load native android external library" << VAR(config_.library_path);
        return false;
    }

    funcs_ = std::move(*funcs_opt);
    last_touch_points_.clear();
    connected_ = true;
    return true;
}

bool AndroidNativeControlUnitMgr::connected() const
{
    return connected_ && funcs_.has_value();
}

bool AndroidNativeControlUnitMgr::request_uuid(std::string& uuid)
{
    uuid = kUuid;
    return true;
}

MaaControllerFeature AndroidNativeControlUnitMgr::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool AndroidNativeControlUnitMgr::start_app(const std::string& intent)
{
    LogFunc << VAR(intent);

    if (!connected()) {
        LogError << "controller is not connected";
        return false;
    }

    MethodParam param { };
    param.display_id = config_.display_id;
    param.method = START_GAME;
    param.args.start_game.package_name = intent.c_str();
    param.args.start_game.force_stop = config_.force_stop ? 1 : 0;
    return dispatch_input_message(param);
}

bool AndroidNativeControlUnitMgr::stop_app(const std::string& intent)
{
    LogFunc << VAR(intent);

    if (!connected()) {
        LogError << "controller is not connected";
        return false;
    }

    MethodParam param { };
    param.display_id = config_.display_id;
    param.method = STOP_GAME;
    param.args.stop_game.client_type = intent.c_str();
    return dispatch_input_message(param);
}

bool AndroidNativeControlUnitMgr::screencap(cv::Mat& image)
{
    LogTrace;

    if (!connected()) {
        LogError << "controller is not connected";
        return false;
    }

    const FrameInfo info = funcs_->get_locked_pixels();
    const bool need_unlock = info.frame_ref != nullptr;

    if (!info.data || info.width == 0 || info.height == 0 || info.stride == 0) {
        if (need_unlock) {
            (void)funcs_->unlock_pixels(info);
        }
        LogError << "GetLockedPixels returned invalid frame info" << VAR(info.width) << VAR(info.height) << VAR(info.stride)
                 << VAR(info.length) << VAR_VOIDP(info.data) << VAR_VOIDP(info.frame_ref);
        return false;
    }

    const cv::Mat bgr(static_cast<int>(info.height), static_cast<int>(info.width), CV_8UC3, info.data, info.stride);
    image = bgr.clone();

    if (need_unlock) {
        if (int unlock_ret = funcs_->unlock_pixels(info); unlock_ret != 0) {
            LogError << "UnlockPixels failed" << VAR(unlock_ret);
            return false;
        }
    }

    return !image.empty();
}

bool AndroidNativeControlUnitMgr::click(int x, int y)
{
    LogError << "get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_up instead"
             << VAR(x) << VAR(y);
    return false;
}

bool AndroidNativeControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_move/touch_up instead"
             << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool AndroidNativeControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    LogFunc << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);
    (void)pressure;

    if (!connected()) {
        LogError << "controller is not connected";
        return false;
    }

    if (!validate_contact(contact)) {
        return false;
    }

    cv::Point mapped;
    if (!normalize_touch_point(x, y, mapped)) {
        return false;
    }

    MethodParam param { };
    param.display_id = config_.display_id;
    param.method = TOUCH_DOWN;
    param.args.touch.p = { mapped.x, mapped.y };

    if (!dispatch_input_message(param)) {
        return false;
    }

    last_touch_points_[contact] = mapped;
    return true;
}

bool AndroidNativeControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    LogFunc << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);
    (void)pressure;

    if (!connected()) {
        LogError << "controller is not connected";
        return false;
    }

    if (!validate_contact(contact)) {
        return false;
    }

    cv::Point mapped;
    if (!normalize_touch_point(x, y, mapped)) {
        return false;
    }

    MethodParam param { };
    param.display_id = config_.display_id;
    param.method = TOUCH_MOVE;
    param.args.touch.p = { mapped.x, mapped.y };

    if (!dispatch_input_message(param)) {
        return false;
    }

    last_touch_points_[contact] = mapped;
    return true;
}

bool AndroidNativeControlUnitMgr::touch_up(int contact)
{
    LogFunc << VAR(contact);

    if (!connected()) {
        LogError << "controller is not connected";
        return false;
    }

    if (!validate_contact(contact)) {
        return false;
    }

    const cv::Point mapped = get_touch_up_point(contact);

    MethodParam param { };
    param.display_id = config_.display_id;
    param.method = TOUCH_UP;
    param.args.touch.p = { mapped.x, mapped.y };

    if (!dispatch_input_message(param)) {
        return false;
    }

    last_touch_points_.erase(contact);
    return true;
}

bool AndroidNativeControlUnitMgr::click_key(int key)
{
    LogError << "get_features() returns MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick, "
                "use key_down/key_up instead"
             << VAR(key);
    return false;
}

bool AndroidNativeControlUnitMgr::input_text(const std::string& text)
{
    LogFunc << VAR(text);

    if (!connected()) {
        LogError << "controller is not connected";
        return false;
    }

    MethodParam param { };
    param.display_id = config_.display_id;
    param.method = INPUT;
    param.args.input.text = text.c_str();
    return dispatch_input_message(param);
}

bool AndroidNativeControlUnitMgr::key_down(int key)
{
    LogFunc << VAR(key);

    if (!connected()) {
        LogError << "controller is not connected";
        return false;
    }

    MethodParam param { };
    param.display_id = config_.display_id;
    param.method = KEY_DOWN;
    param.args.key.key_code = key;
    return dispatch_input_message(param);
}

bool AndroidNativeControlUnitMgr::key_up(int key)
{
    LogFunc << VAR(key);

    if (!connected()) {
        LogError << "controller is not connected";
        return false;
    }

    MethodParam param { };
    param.display_id = config_.display_id;
    param.method = KEY_UP;
    param.args.key.key_code = key;
    return dispatch_input_message(param);
}

bool AndroidNativeControlUnitMgr::inactive()
{
    return true;
}

json::object AndroidNativeControlUnitMgr::get_info() const
{
    json::object info;
    info["type"] = "native_android";
    info["library_path"] = path_to_utf8_string(config_.library_path);
    info["display_id"] = config_.display_id;
    info["force_stop"] = config_.force_stop;
    info["loaded"] = funcs_.has_value();
    info["connected"] = connected();

    json::object touch_resolution;
    touch_resolution["width"] = config_.touch_width;
    touch_resolution["height"] = config_.touch_height;
    info["touch_resolution"] = std::move(touch_resolution);
    return info;
}

bool AndroidNativeControlUnitMgr::validate_contact(int contact)
{
    if (contact == 0) {
        return true;
    }

    LogWarn << "native android controller only supports single touch" << VAR(contact);
    return false;
}

bool AndroidNativeControlUnitMgr::dispatch_input_message(MethodParam param) const
{
    int ret = funcs_->dispatch_input_message(param);
    if (ret == 0) {
        return true;
    }

    LogError << "DispatchInputMessage failed" << VAR(ret) << VAR(param.display_id) << VAR(param.method);
    return false;
}

bool AndroidNativeControlUnitMgr::normalize_touch_point(int raw_x, int raw_y, cv::Point& mapped) const
{
    if (config_.touch_width <= 0 || config_.touch_height <= 0) {
        LogError << "touch resolution is invalid" << VAR(config_.touch_width) << VAR(config_.touch_height);
        return false;
    }

    mapped.x = std::clamp(raw_x, 0, std::max(0, config_.touch_width - 1));
    mapped.y = std::clamp(raw_y, 0, std::max(0, config_.touch_height - 1));
    return true;
}

cv::Point AndroidNativeControlUnitMgr::get_touch_up_point(int contact) const
{
    if (const auto it = last_touch_points_.find(contact); it != last_touch_points_.end()) {
        return it->second;
    }

    LogWarn << "touch_up without previous touch point, fallback to center" << VAR(contact);
    return {
        std::max(0, config_.touch_width / 2),
        std::max(0, config_.touch_height / 2),
    };
}

MAA_CTRL_UNIT_NS_END
