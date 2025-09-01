#ifdef _WIN32

#include "MuMuPlayerExtras.h"

#include "Utils/Encoding.h"
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
        lib_path_ = mumu_path_ / MAA_NS::path("nx_main/sdk/external_renderer_ipc.dll");
        if (!std::filesystem::exists(lib_path_)) {
            lib_path_ = mumu_path_ / MAA_NS::path("shell/sdk/external_renderer_ipc.dll");
            if (!std::filesystem::exists(lib_path_)) {
                LogError << "Failed to find mumu library path, please check extras.mumu.lib or extras.mumu.path";
                return false;
            }
        }
    }
    else {
        lib_path_ = MAA_NS::path(lib);
    }

    mumu_index_ = config.get("extras", "mumu", "index", 0);

    std::string package = config.get("extras", "mumu", "app_package", "");
    int cloned_index = config.get("extras", "mumu", "app_cloned_index", 0);
    set_app_package(package, cloned_index);

    LogInfo << VAR(mumu_path_) << VAR(lib_path_) << VAR(mumu_index_);
    return true;
}

bool MuMuPlayerExtras::init()
{
    return load_mumu_library() && connect_mumu() && init_screencap();
}

std::optional<cv::Mat> MuMuPlayerExtras::screencap()
{
    LogDebug;

    if (!capture_display_func_) {
        LogError << "capture_display_func_ is null";
        return std::nullopt;
    }

    int display_id = get_display_id();
    int ret = capture_display_func_(
        mumu_handle_,
        display_id,
        static_cast<int>(display_buffer_.size()),
        &display_width_,
        &display_height_,
        display_buffer_.data());

    if (ret) {
        LogError << "Failed to capture display" << VAR(ret) << VAR(mumu_handle_) << VAR(display_id) << VAR(display_buffer_.size())
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
    LogError << "deprecated" << VAR(x) << VAR(y);
    return false;
}

bool MuMuPlayerExtras::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated" << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool MuMuPlayerExtras::touch_down(int contact, int x, int y, int pressure)
{
    if (!input_event_touch_down_func_) {
        LogError << "input_event_touch_down_func_ is null";
        return false;
    }

    int display_id = get_display_id();
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure) << VAR(display_id);

    // contact start from 0, but mumu start from 1
    int ret = input_event_touch_down_func_(mumu_handle_, display_id, contact + 1, x, y);

    if (ret != 0) {
        LogError << "Failed to touch_down" << VAR(ret);
        return false;
    }

    return true;
}

bool MuMuPlayerExtras::touch_move(int contact, int x, int y, int pressure)
{
    // mumu: touch_down == touch_move

    std::ignore = contact;
    std::ignore = pressure;

    if (!input_event_touch_down_func_) {
        LogError << "input_event_touch_down_func_ is null";
        return false;
    }

    int display_id = get_display_id();
    // LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure) << VAR(display_id);

    // contact start from 0, but mumu start from 1
    int ret = input_event_touch_down_func_(mumu_handle_, display_id, contact + 1, x, y);

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

    int display_id = get_display_id();
    LogInfo << VAR(contact) << VAR(display_id);

    // contact start from 0, but mumu start from 1
    int ret = input_event_touch_up_func_(mumu_handle_, display_id, contact + 1);

    if (ret != 0) {
        LogError << "Failed to touch_up" << VAR(ret);
        return false;
    }

    return true;
}

bool MuMuPlayerExtras::click_key(int key)
{
    LogError << "deprecated" << VAR(key);
    return false;
}

bool MuMuPlayerExtras::input_text(const std::string& text)
{
    if (!input_text_func_) {
        LogError << "input_text_func_ is null";
        return false;
    }

    int display_id = get_display_id();
    LogInfo << VAR(text) << VAR(display_id);

    int ret = input_text_func_(mumu_handle_, display_id, text.c_str());

    if (ret != 0) {
        LogError << "Failed to input_text" << VAR(ret);
        return false;
    }

    return true;
}

bool MuMuPlayerExtras::key_down(int key)
{
    if (!input_event_key_down_func_) {
        LogError << "input_event_key_down_func_ is null";
        return false;
    }

    int display_id = get_display_id();
    LogInfo << VAR(key) << VAR(display_id);

    int down_ret = input_event_key_down_func_(mumu_handle_, display_id, key);

    if (down_ret != 0) {
        LogError << "Failed to key_down" << VAR(down_ret);
        return false;
    }

    return true;
}

bool MuMuPlayerExtras::key_up(int key)
{
    if (!input_event_key_up_func_) {
        LogError << "input_event_key_up_func_ is null";
        return false;
    }

    int display_id = get_display_id();
    LogInfo << VAR(key) << VAR(display_id);

    int up_ret = input_event_key_up_func_(mumu_handle_, display_id, key);

    if (up_ret != 0) {
        LogError << "Failed to click_key" << VAR(up_ret);
        return false;
    }

    return true;
}

void MuMuPlayerExtras::on_app_started(const std::string& intent)
{
    std::string package = string_split(intent, '/').front();

    set_app_package(package, 0);
}

void MuMuPlayerExtras::on_app_stopped(const std::string& intent)
{
    std::ignore = intent;

    set_app_package("", 0);
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

    input_event_touch_down_func_ = get_function<decltype(nemu_input_event_finger_touch_down)>(kInputEventTouchDownFuncName);
    if (!input_event_touch_down_func_) {
        LogError << "Failed to get function" << VAR(kInputEventTouchDownFuncName);
        return false;
    }

    input_event_touch_up_func_ = get_function<decltype(nemu_input_event_finger_touch_up)>(kInputEventTouchUpFuncName);
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

    if (mumu_handle_) {
        LogWarn << "mumu_handle_ is already connected" << VAR(mumu_handle_);
        disconnect_mumu();
    }

    std::u16string u16path = mumu_path_.u16string();
    std::wstring wpath(std::make_move_iterator(u16path.begin()), std::make_move_iterator(u16path.end()));

    mumu_handle_ = connect_func_(wpath.c_str(), mumu_index_);

    if (mumu_handle_ == 0) {
        LogError << "Failed to connect mumu" << VAR(wpath) << VAR(mumu_index_);
        return false;
    }

    ++s_mumu_handle_refs_[mumu_handle_];

    return true;
}

bool MuMuPlayerExtras::init_screencap()
{
    if (!capture_display_func_) {
        LogError << "capture_display_func_ is null";
        return false;
    }

    clear_display_id();
    int display_id = get_display_id();
    int ret = capture_display_func_(mumu_handle_, display_id, 0, &display_width_, &display_height_, nullptr);

    // mumu 的文档给错了，这里 0 才是成功
    if (ret) {
        LogError << "Failed to capture display" << VAR(ret) << VAR(mumu_handle_) << VAR(display_id);
        return false;
    }

    display_buffer_.resize(display_width_ * display_height_ * 4);

    LogDebug << VAR(display_width_) << VAR(display_height_) << VAR(display_buffer_.size());

    return true;
}

void MuMuPlayerExtras::disconnect_mumu()
{
    LogFunc << VAR(mumu_handle_);

    if (!disconnect_func_) {
        LogError << "disconnect_func_ is null";
        return;
    }

    int ref = --s_mumu_handle_refs_[mumu_handle_];
    if (ref > 0) {
        LogDebug << "mumu_handle_ still has references" << VAR(ref);
        return;
    }
    s_mumu_handle_refs_.erase(mumu_handle_);

    if (mumu_handle_ != 0) {
        disconnect_func_(mumu_handle_);
    }

    mumu_handle_ = 0;
}

void MuMuPlayerExtras::set_app_package(const std::string& package, int cloned_index)
{
    LogDebug << VAR(package) << VAR(cloned_index);

    clear_display_id();

    mumu_app_package_ = package;
    mumu_app_cloned_index_ = cloned_index;
}

void MuMuPlayerExtras::clear_display_id()
{
    mumu_display_id_cache_ = std::nullopt;
}

int MuMuPlayerExtras::get_display_id()
{
    if (mumu_display_id_cache_) {
        return *mumu_display_id_cache_;
    }

    if (!get_display_id_func_) {
        LogError << "get_display_id_func_ is null";
        return 0;
    }

    int display_id = 0;
    if (!mumu_app_package_.empty()) {
        display_id = get_display_id_func_(mumu_handle_, mumu_app_package_.c_str(), mumu_app_cloned_index_);
    }
    else {
        static const std::string kDefaultPkg = "default";
        display_id = get_display_id_func_(mumu_handle_, kDefaultPkg.c_str(), 0);
    }
    LogInfo << VAR(display_id);
    if (display_id < 0) {
        LogError << "Failed to get display id" << VAR(display_id);
        return 0;
    }

    mumu_display_id_cache_ = display_id;
    return display_id;
}

MAA_CTRL_UNIT_NS_END

#endif
