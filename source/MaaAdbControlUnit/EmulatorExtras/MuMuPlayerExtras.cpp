#ifdef _WIN32

#include "MuMuPlayerExtras.h"

#include "ControlUnit/MicroControl.hpp"
#include "Utils/Codec.h"
#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"
#include "Utils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

MuMuPlayerExtras::~MuMuPlayerExtras()
{
    disconnect_mumu();
}

bool MuMuPlayerExtras::parse(const json::value& config)
{
    bool enable = config.get("extras", "mumu", "enable", false);
    if (!enable) {
        LogInfo << "extras.mumu.enable is false, ignore";
        return false;
    }

    std::string p = config.get("extras", "mumu", "path", "");
    if (p.empty()) {
        LogError << "falied to get mumu path from config: extras.mumu.path";
        return false;
    }
    mumu_path_ = MAA_NS::path(p);

    std::string lib = config.get("extras", "mumu", "lib", "");
    if (lib.empty()) {
        lib_path_ = mumu_path_ / MAA_NS::path("shell/sdk/external_renderer_ipc");
    }
    else {
        lib_path_ = MAA_NS::path(lib);
    }

    mumu_index_ = config.get("extras", "mumu", "index", 0);
    mumu_display_id_ = config.get("extras", "mumu", "display_id", -1);

    mumu_app_package_ = config.get("extras", "mumu", "app_package", "");
    mumu_app_cloned_index_ = config.get("extras", "mumu", "app_cloned_index", 0);

    LogInfo << VAR(mumu_path_) << VAR(lib_path_) << VAR(mumu_index_) << VAR(mumu_display_id_);
    return true;
}

bool MuMuPlayerExtras::init()
{
    return load_mumu_library() && connect_mumu() && init_display_id() && init_screencap();
}

void MuMuPlayerExtras::deinit()
{
    disconnect_mumu();
}

std::optional<cv::Mat> MuMuPlayerExtras::screencap()
{
    LogDebug;

    if (!capture_display_func_) {
        LogError << "capture_display_func_ is null";
        return std::nullopt;
    }

    int ret = capture_display_func_(
        mumu_handle_,
        mumu_display_id_,
        static_cast<int>(display_buffer_.size()),
        &display_width_,
        &display_height_,
        display_buffer_.data());

    if (ret) {
        LogError << "Failed to capture display" << VAR(ret) << VAR(mumu_handle_) << VAR(mumu_display_id_) << VAR(display_buffer_.size())
                 << VAR(display_width_) << VAR(display_height_);
        return std::nullopt;
    }

    cv::Mat raw(display_height_, display_width_, CV_8UC4, display_buffer_.data());
    cv::Mat bgr;
    cv::cvtColor(raw, bgr, cv::COLOR_RGBA2BGR);
    cv::Mat dst;
    cv::flip(bgr, dst, 0);

    return dst;
}

bool MuMuPlayerExtras::click(int x, int y)
{
    if (!input_event_touch_down_func_ || !input_event_touch_up_func_) {
        LogError << "input_event_touch_down_func_ or input_event_touch_up_func_ is null";
        return false;
    }

    LogInfo << VAR(x) << VAR(y);

    int down_ret = input_event_touch_down_func_(mumu_handle_, mumu_display_id_, x, y);
    int up_ret = input_event_touch_up_func_(mumu_handle_, mumu_display_id_);

    if (down_ret != 0 || up_ret != 0) {
        LogError << "Failed to click" << VAR(down_ret) << VAR(up_ret);
        return false;
    }

    return true;
}

bool MuMuPlayerExtras::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!input_event_touch_down_func_ || !input_event_touch_up_func_) {
        LogError << "input_event_touch_down_func_ or input_event_touch_up_func_ is null";
        return false;
    }

    if (duration <= 0) {
        LogWarn << "duration out of range" << VAR(duration);
        duration = 500;
    }

    LogInfo << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    int ret = 0;

    micro_swipe(
        x1,
        y1,
        x2,
        y2,
        duration,
        [&](int x, int y) { ret |= input_event_touch_down_func_(mumu_handle_, mumu_display_id_, x, y); },
        [&](int x, int y) { ret |= input_event_touch_down_func_(mumu_handle_, mumu_display_id_, x, y); },
        [&]([[maybe_unused]] int x, [[maybe_unused]] int y) { ret |= input_event_touch_up_func_(mumu_handle_, mumu_display_id_); });

    if (ret != 0) {
        LogError << "Failed to swipe" << VAR(ret);
        return false;
    }

    return true;
}

bool MuMuPlayerExtras::touch_down(int contact, int x, int y, int pressure)
{
    if (!input_event_touch_down_func_) {
        LogError << "input_event_touch_down_func_ is null";
        return false;
    }

    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    int ret = input_event_touch_down_func_(mumu_handle_, mumu_display_id_, x, y);

    if (ret != 0) {
        LogError << "Failed to touch_down" << VAR(ret);
        return false;
    }

    return true;
}

bool MuMuPlayerExtras::touch_move(int contact, int x, int y, int pressure)
{
    // mumu: touch_down == touch_move

    if (!input_event_touch_down_func_) {
        LogError << "input_event_touch_down_func_ is null";
        return false;
    }

    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    int ret = input_event_touch_down_func_(mumu_handle_, mumu_display_id_, x, y);

    if (ret != 0) {
        LogError << "Failed to touch_down" << VAR(ret);
        return false;
    }

    return true;
}

bool MuMuPlayerExtras::touch_up(int contact)
{
    if (!input_event_touch_up_func_) {
        LogError << "input_event_touch_up_func_ is null";
        return false;
    }

    LogInfo << VAR(contact);

    int ret = input_event_touch_up_func_(mumu_handle_, mumu_display_id_);

    if (ret != 0) {
        LogError << "Failed to touch_up" << VAR(ret);
        return false;
    }

    return true;
}

bool MuMuPlayerExtras::press_key(int key)
{
    if (!input_event_key_down_func_ || !input_event_key_up_func_) {
        LogError << "input_event_key_down_func_ or input_event_key_up_func_ is null";
        return false;
    }

    LogInfo << VAR(key);

    int down_ret = input_event_key_down_func_(mumu_handle_, mumu_display_id_, key);
    int up_ret = input_event_key_up_func_(mumu_handle_, mumu_display_id_, key);

    if (down_ret != 0 || up_ret != 0) {
        LogError << "Failed to press_key" << VAR(down_ret) << VAR(up_ret);
        return false;
    }

    return true;
}

bool MuMuPlayerExtras::input_text(const std::string& text)
{
    if (!input_text_func_) {
        LogError << "input_text_func_ is null";
        return false;
    }

    LogInfo << VAR(text);

    int ret = input_text_func_(mumu_handle_, mumu_display_id_, text.c_str());

    if (ret != 0) {
        LogError << "Failed to input_text" << VAR(ret);
        return false;
    }

    return true;
}

bool MuMuPlayerExtras::load_mumu_library()
{
    if (!load_library(lib_path_)) {
        LogError << "Failed to load library" << VAR(lib_path_);
        return false;
    }

    connect_func_ = get_function<decltype(nemu_connect)>(kConnectFuncName);
    if (!connect_func_) {
        LogError << "Failed to get function" << VAR(kConnectFuncName);
        return false;
    }

    disconnect_func_ = get_function<decltype(nemu_disconnect)>(kDisconnectFuncName);
    if (!disconnect_func_) {
        LogError << "Failed to get function" << VAR(kDisconnectFuncName);
        return false;
    }

    capture_display_func_ = get_function<decltype(nemu_capture_display)>(kCaptureDisplayFuncName);
    if (!capture_display_func_) {
        LogError << "Failed to get function" << VAR(kCaptureDisplayFuncName);
        return false;
    }

    input_text_func_ = get_function<decltype(nemu_input_text)>(kInputTextFuncName);
    if (!input_text_func_) {
        LogError << "Failed to get function" << VAR(kInputTextFuncName);
        return false;
    }

    input_event_touch_down_func_ = get_function<decltype(nemu_input_event_touch_down)>(kInputEventTouchDownFuncName);
    if (!input_event_touch_down_func_) {
        LogError << "Failed to get function" << VAR(kInputEventTouchDownFuncName);
        return false;
    }

    input_event_touch_up_func_ = get_function<decltype(nemu_input_event_touch_up)>(kInputEventTouchUpFuncName);
    if (!input_event_touch_up_func_) {
        LogError << "Failed to get function" << VAR(kInputEventTouchUpFuncName);
        return false;
    }

    input_event_key_down_func_ = get_function<decltype(nemu_input_event_key_down)>(kInputEventKeyDownFuncName);
    if (!input_event_key_down_func_) {
        LogError << "Failed to get function" << VAR(kInputEventKeyDownFuncName);
        return false;
    }

    input_event_key_up_func_ = get_function<decltype(nemu_input_event_key_up)>(kInputEventKeyUpFuncName);
    if (!input_event_key_up_func_) {
        LogError << "Failed to get function" << VAR(kInputEventKeyUpFuncName);
        return false;
    }

    get_display_id_func_ = get_function<decltype(nemu_get_display_id)>(kGetDisplayIdFuncName);
    if (!get_display_id_func_) {
        LogError << "Failed to get function" << VAR(kGetDisplayIdFuncName);
        return false;
    }

    return true;
}

bool MuMuPlayerExtras::connect_mumu()
{
    LogFunc << VAR(mumu_path_) << VAR(mumu_index_);

    if (!connect_func_) {
        LogError << "connect_func_ is null";
        return false;
    }

    std::u16string u16path = mumu_path_.u16string();
    std::wstring wpath(std::make_move_iterator(u16path.begin()), std::make_move_iterator(u16path.end()));

    mumu_handle_ = connect_func_(wpath.c_str(), mumu_index_);

    if (mumu_handle_ == 0) {
        LogError << "Failed to connect mumu" << VAR(wpath) << VAR(mumu_index_);
        return false;
    }

    return true;
}

bool MuMuPlayerExtras::init_screencap()
{
    if (!capture_display_func_) {
        LogError << "capture_display_func_ is null";
        return false;
    }

    int ret = capture_display_func_(mumu_handle_, mumu_display_id_, 0, &display_width_, &display_height_, nullptr);

    // mumu 的文档给错了，这里 0 才是成功
    if (ret) {
        LogError << "Failed to capture display" << VAR(ret) << VAR(mumu_handle_) << VAR(mumu_display_id_);
        return false;
    }

    display_buffer_.resize(display_width_ * display_height_ * 4);

    LogDebug << VAR(display_width_) << VAR(display_height_) << VAR(display_buffer_.size());

    return true;
}

bool MuMuPlayerExtras::init_display_id()
{
    if (mumu_display_id_ >= 0) {
        return true;
    }

    if (!get_display_id_func_) {
        LogError << "get_display_id_func_ is null";
        return false;
    }

    if (!mumu_app_package_.empty()) {
        mumu_display_id_ = get_display_id_func_(mumu_handle_, mumu_app_package_.c_str(), mumu_app_cloned_index_);
    }
    else {
        static const std::string kDefaultPkg = "default";
        mumu_display_id_ = get_display_id_func_(mumu_handle_, kDefaultPkg.c_str(), 0);
    }
    LogInfo << VAR(mumu_display_id_);

    return mumu_display_id_ >= 0;
}

void MuMuPlayerExtras::disconnect_mumu()
{
    LogFunc << VAR(mumu_handle_);

    if (mumu_handle_ != 0) {
        disconnect_func_(mumu_handle_);
    }
}

MAA_CTRL_UNIT_NS_END

#endif
