#ifdef _WIN32

#include "MuMuPlayerExtras.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "MaaUtils/Platform.h"

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

void MuMuPlayerExtras::on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur)
{
    std::ignore = pre;
    std::ignore = cur;

    disconnect_mumu();
    connect_mumu() && init_screencap();
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

MaaControllerFeature MuMuPlayerExtras::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
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

    LogInfo << VAR(text) << VAR(text.size());

    int ret = input_text_func_(mumu_handle_, static_cast<int>(text.size()), text.c_str());

    if (ret != 0) {
        LogError << "Failed to input_text" << VAR(ret);
        return false;
    }

    return true;
}

int MuMuPlayerExtras::android_keycode_to_linux_key_code(int key)
{
    // https://developer.android.com/reference/android/view/KeyEvent
    // https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
    static const std::unordered_map<int, int> kMap = {
        // Letters
        { 29, 30 }, // KEYCODE_A -> KEY_A
        { 30, 48 }, // KEYCODE_B -> KEY_B
        { 31, 46 }, // KEYCODE_C -> KEY_C
        { 32, 32 }, // KEYCODE_D -> KEY_D
        { 33, 18 }, // KEYCODE_E -> KEY_E
        { 34, 33 }, // KEYCODE_F -> KEY_F
        { 35, 34 }, // KEYCODE_G -> KEY_G
        { 36, 35 }, // KEYCODE_H -> KEY_H
        { 37, 23 }, // KEYCODE_I -> KEY_I
        { 38, 36 }, // KEYCODE_J -> KEY_J
        { 39, 37 }, // KEYCODE_K -> KEY_K
        { 40, 38 }, // KEYCODE_L -> KEY_L
        { 41, 50 }, // KEYCODE_M -> KEY_M
        { 42, 49 }, // KEYCODE_N -> KEY_N
        { 43, 24 }, // KEYCODE_O -> KEY_O
        { 44, 25 }, // KEYCODE_P -> KEY_P
        { 45, 16 }, // KEYCODE_Q -> KEY_Q
        { 46, 19 }, // KEYCODE_R -> KEY_R
        { 47, 31 }, // KEYCODE_S -> KEY_S
        { 48, 20 }, // KEYCODE_T -> KEY_T
        { 49, 22 }, // KEYCODE_U -> KEY_U
        { 50, 47 }, // KEYCODE_V -> KEY_V
        { 51, 17 }, // KEYCODE_W -> KEY_W
        { 52, 45 }, // KEYCODE_X -> KEY_X
        { 53, 21 }, // KEYCODE_Y -> KEY_Y
        { 54, 44 }, // KEYCODE_Z -> KEY_Z

        // Numbers (top row)
        { 7, 11 },  // KEYCODE_0 -> KEY_0
        { 8, 2 },   // KEYCODE_1 -> KEY_1
        { 9, 3 },   // KEYCODE_2 -> KEY_2
        { 10, 4 },  // KEYCODE_3 -> KEY_3
        { 11, 5 },  // KEYCODE_4 -> KEY_4
        { 12, 6 },  // KEYCODE_5 -> KEY_5
        { 13, 7 },  // KEYCODE_6 -> KEY_6
        { 14, 8 },  // KEYCODE_7 -> KEY_7
        { 15, 9 },  // KEYCODE_8 -> KEY_8
        { 16, 10 }, // KEYCODE_9 -> KEY_9

        // Function keys
        { 131, 59 }, // KEYCODE_F1 -> KEY_F1
        { 132, 60 }, // KEYCODE_F2 -> KEY_F2
        { 133, 61 }, // KEYCODE_F3 -> KEY_F3
        { 134, 62 }, // KEYCODE_F4 -> KEY_F4
        { 135, 63 }, // KEYCODE_F5 -> KEY_F5
        { 136, 64 }, // KEYCODE_F6 -> KEY_F6
        { 137, 65 }, // KEYCODE_F7 -> KEY_F7
        { 138, 66 }, // KEYCODE_F8 -> KEY_F8
        { 139, 67 }, // KEYCODE_F9 -> KEY_F9
        { 140, 68 }, // KEYCODE_F10 -> KEY_F10
        { 141, 87 }, // KEYCODE_F11 -> KEY_F11
        { 142, 88 }, // KEYCODE_F12 -> KEY_F12

        // Navigation
        { 19, 103 }, // KEYCODE_DPAD_UP -> KEY_UP
        { 20, 108 }, // KEYCODE_DPAD_DOWN -> KEY_DOWN
        { 21, 105 }, // KEYCODE_DPAD_LEFT -> KEY_LEFT
        { 22, 106 }, // KEYCODE_DPAD_RIGHT -> KEY_RIGHT
        { 23, 28 },  // KEYCODE_DPAD_CENTER -> KEY_ENTER

        // Space, Enter, Backspace, Tab, Escape
        { 62, 57 }, // KEYCODE_SPACE -> KEY_SPACE
        { 66, 28 }, // KEYCODE_ENTER -> KEY_ENTER
        { 67, 14 }, // KEYCODE_DEL -> KEY_BACKSPACE
        { 61, 15 }, // KEYCODE_TAB -> KEY_TAB
        { 111, 1 }, // KEYCODE_ESCAPE -> KEY_ESC

        // Shift, Ctrl, Alt, CapsLock, Meta
        { 59, 42 },   // KEYCODE_SHIFT_LEFT -> KEY_LEFTSHIFT
        { 60, 54 },   // KEYCODE_SHIFT_RIGHT -> KEY_RIGHTSHIFT
        { 113, 29 },  // KEYCODE_CTRL_LEFT -> KEY_LEFTCTRL
        { 114, 97 },  // KEYCODE_CTRL_RIGHT -> KEY_RIGHTCTRL
        { 57, 56 },   // KEYCODE_ALT_LEFT -> KEY_LEFTALT
        { 58, 100 },  // KEYCODE_ALT_RIGHT -> KEY_RIGHTALT
        { 115, 58 },  // KEYCODE_CAPS_LOCK -> KEY_CAPSLOCK
        { 117, 125 }, // KEYCODE_META_LEFT -> KEY_LEFTMETA
        { 118, 126 }, // KEYCODE_META_RIGHT -> KEY_RIGHTMETA

        // Symbols
        { 69, 15 },   // KEYCODE_TAB -> KEY_TAB
        { 68, 15 },   // KEYCODE_TAB (duplicate, for compatibility)
        { 76, 127 },  // KEYCODE_MOVE_HOME -> KEY_HOME
        { 122, 102 }, // KEYCODE_MOVE_HOME -> KEY_HOME
        { 123, 107 }, // KEYCODE_MOVE_END -> KEY_END
        { 124, 104 }, // KEYCODE_PAGE_UP -> KEY_PAGEUP
        { 92, 109 },  // KEYCODE_PAGE_DOWN -> KEY_PAGEDOWN
        { 112, 111 }, // KEYCODE_FORWARD_DEL -> KEY_DELETE

        // Misc
        { 4, 158 },   // KEYCODE_BACK -> KEY_BACK
        { 3, 102 },   // KEYCODE_HOME -> KEY_HOME
        { 82, 139 },  // KEYCODE_MENU -> KEY_MENU
        { 84, 114 },  // KEYCODE_SEARCH -> KEY_SEARCH
        { 85, 164 },  // KEYCODE_MEDIA_PLAY_PAUSE -> KEY_PLAYPAUSE
        { 86, 128 },  // KEYCODE_MEDIA_STOP -> KEY_STOPCD
        { 87, 163 },  // KEYCODE_MEDIA_NEXT -> KEY_NEXTSONG
        { 88, 165 },  // KEYCODE_MEDIA_PREVIOUS -> KEY_PREVIOUSSONG
        { 89, 168 },  // KEYCODE_MEDIA_REWIND -> KEY_REWIND
        { 90, 208 },  // KEYCODE_MEDIA_FAST_FORWARD -> KEY_FASTFORWARD
        { 24, 115 },  // KEYCODE_VOLUME_UP -> KEY_VOLUMEUP
        { 25, 114 },  // KEYCODE_VOLUME_DOWN -> KEY_VOLUMEDOWN
        { 164, 113 }, // KEYCODE_VOLUME_MUTE -> KEY_MUTE
    };

    auto it = kMap.find(key);
    if (it == kMap.end()) {
        LogWarn << "unknown android key" << VAR(key);
        return key;
    }
    return it->second;
}

bool MuMuPlayerExtras::key_down(int key)
{
    if (!input_event_key_down_func_) {
        LogError << "input_event_key_down_func_ is null";
        return false;
    }

    int display_id = get_display_id();
    int linux_key = android_keycode_to_linux_key_code(key);
    LogInfo << VAR(key) << VAR(linux_key) << VAR(display_id);

    int down_ret = input_event_key_down_func_(mumu_handle_, display_id, linux_key);

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
    int linux_key = android_keycode_to_linux_key_code(key);
    LogInfo << VAR(key) << VAR(linux_key) << VAR(display_id);

    int up_ret = input_event_key_up_func_(mumu_handle_, display_id, linux_key);

    if (up_ret != 0) {
        LogError << "Failed to click_key" << VAR(up_ret);
        return false;
    }

    return true;
}

bool MuMuPlayerExtras::scroll(int dx, int dy)
{
    LogError << "Scroll is not supported on MuMuPlayerExtras" << VAR(dx) << VAR(dy);
    return false;
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
