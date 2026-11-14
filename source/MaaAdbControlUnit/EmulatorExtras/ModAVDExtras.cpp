#ifndef __ANDROID__

#include "ModAVDExtras.h"

#include <chrono>
#include <thread>

#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "MaaUtils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

ModAVDExtras::ModAVDExtras(Usage usage)
    : usage_(usage)
{
    // Keys and text fall back to adb shell input, so only the input unit needs it.
    if (usage_ == Usage::Input) {
        adb_shell_input_ = std::make_shared<AdbShellInput>();
        children_.emplace_back(adb_shell_input_);
    }
}

ModAVDExtras::~ModAVDExtras()
{
    close_instance();
    unload_modavd_library();
}

bool ModAVDExtras::parse(const json::value& config)
{
    enabled_ = config.get("extras", "modavd", "enable", false);
    if (!enabled_) {
        LogInfo << "extras.modavd.enable is false, ignore";
        return false;
    }

    std::string lib = config.get("extras", "modavd", "lib", "");
    if (!lib.empty()) {
        lib_path_ = MAA_NS::path(lib);
    }
    else {
        std::string root = config.get("extras", "modavd", "path", "");
        if (root.empty()) {
            LogError << "failed to get ModAVD path from config: extras.modavd.path or extras.modavd.lib";
            enabled_ = false;
            return false;
        }
        // LibraryHolder adds the platform decorations: modavd_extras.dll, libmodavd_extras.so, libmodavd_extras.dylib
        lib_path_ = MAA_NS::path(root) / MAA_NS::path("extras") / MAA_NS::path("modavd_extras");
    }

    // Empty address: use the adb serial. console_port 0: the library derives it from the address.
    address_ = config.get("extras", "modavd", "address", "");
    console_port_ = config.get("extras", "modavd", "console_port", 0);

    LogInfo << VAR(lib_path_) << VAR(address_) << VAR(console_port_) << VAR(usage_ == Usage::Input);

    if (adb_shell_input_ && !adb_shell_input_->parse(config)) {
        enabled_ = false;
        return false;
    }
    return true;
}

bool ModAVDExtras::init()
{
    // InputAgent tries every unit, including ones whose parse failed
    if (!enabled_) {
        return false;
    }

    if (address_.empty()) {
        if (auto it = argv_replace_.find("{ADB_SERIAL}"); it != argv_replace_.end()) {
            address_ = it->second;
        }
    }
    if (address_.empty() && console_port_ <= 0) {
        LogError << "ModAVD address is empty";
        return false;
    }

    if (!(library_loaded_ || load_modavd_library()) || !open_instance()) {
        return false;
    }

    if (usage_ == Usage::Screencap && !(capabilities_ & kCapScreencap)) {
        LogError << "ModAVD instance cannot capture" << VAR(address_) << VAR(capabilities_);
        close_instance();
        return false;
    }

    if (usage_ == Usage::Input) {
        // Touch is what makes this unit worth using; without it, let InputAgent pick the next method.
        if (!(capabilities_ & kCapTouch) || !touch_down_func_ || !touch_move_func_ || !touch_up_func_) {
            LogWarn << "ModAVD extras has no touch input" << VAR(api_version_) << VAR(capabilities_);
            close_instance();
            return false;
        }
        LogInfo << "ModAVD extras input" << VAR(capabilities_) << VAR(bool(capabilities_ & kCapKey)) << VAR(bool(capabilities_ & kCapText));
    }

    return true;
}

std::optional<cv::Mat> ModAVDExtras::screencap()
{
    // A failed reopen (e.g. after a resolution change) leaves no handle; try once more here.
    if (!handle_ && library_loaded_) {
        reopen_instance();
    }
    if (!handle_ || !capture_func_) {
        LogError << "ModAVD instance is not opened";
        return std::nullopt;
    }

    int ret = kOk;
    auto image = capture_frame(ret);
    if (image || ret == kOk) {
        return image;
    }

    // MODAVD_E_RESIZED needs a new handle; for the other errors one reopen is cheap and
    // saves AdbControlUnitMgr from retrying with delays.
    if (!reopen_instance()) {
        LogError << "ModAVD capture failed and reopen failed" << VAR(ret) << VAR(error_string(ret));
        return std::nullopt;
    }

    image = capture_frame(ret);
    if (!image) {
        LogError << "ModAVD capture failed after reopen" << VAR(ret) << VAR(error_string(ret));
    }
    return image;
}

std::optional<cv::Mat> ModAVDExtras::capture_frame(int& ret)
{
    auto image = capture_once(ret);

    // MODAVD_E_NOT_READY: no frame has arrived since sharing started, or new frames kept
    // arriving during the copy. Both clear up quickly.
    using namespace std::chrono_literals;
    for (int i = 0; !image && ret == kErrNotReady && i < kNotReadyRetries; ++i) {
        std::this_thread::sleep_for(50ms);
        image = capture_once(ret);
    }
    return image;
}

std::optional<cv::Mat> ModAVDExtras::capture_once(int& ret)
{
    // Probe the size, then copy straight into a BGR Mat: top-down rows, no conversion needed.
    FrameInfo info { };
    info.size = sizeof(info);
    ret = capture_func_(handle_, 0, kFormatBgr, 0, nullptr, 0, &info);
    if (ret != kOk || info.width <= 0 || info.height <= 0) {
        if (ret != kErrNotReady) {
            LogWarn << "ModAVD size probe failed" << VAR(ret) << VAR(error_string(ret)) << VAR(info.width) << VAR(info.height);
        }
        return std::nullopt;
    }

    cv::Mat image(info.height, info.width, CV_8UC3);
    if (static_cast<size_t>(info.stride) != image.step[0]) {
        LogError << "ModAVD unexpected stride" << VAR(info.stride) << VAR(image.step[0]);
        ret = kOk;
        return std::nullopt;
    }

    ret = capture_func_(handle_, 0, kFormatBgr, 0, image.data, image.total() * image.elemSize(), &info);
    if (ret != kOk) {
        LogWarn << "ModAVD capture failed" << VAR(ret) << VAR(error_string(ret));
        return std::nullopt;
    }

    return image;
}

MaaControllerFeature ModAVDExtras::get_features() const
{
    MaaControllerFeature feat = MaaControllerFeature_UseMouseDownAndUpInsteadOfClick;
    if (capabilities_ & kCapKey) {
        feat |= MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
    }
    return feat;
}

// get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick,
// ControllerAgent uses touch_down/touch_move/touch_up instead of click/swipe
bool ModAVDExtras::click(int x, int y)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_up instead"
             << VAR(x) << VAR(y);
    return false;
}

bool ModAVDExtras::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_move/touch_up instead"
             << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool ModAVDExtras::touch_down(int contact, int x, int y, int pressure)
{
    if (!handle_ && library_loaded_) {
        reopen_instance();
    }
    if (!handle_ || !touch_down_func_) {
        LogError << "ModAVD touch is not available";
        return false;
    }

    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    // Coordinates are display pixels of the shared frame, the same as the screencap.
    // Pressure 0 is the library's default; the scale of MaaFramework's pressure does not apply here.
    int ret = touch_down_func_(handle_, 0, contact, x, y, 0);
    if (ret != kOk) {
        LogError << "Failed to touch_down" << VAR(ret) << VAR(error_string(ret));
        return false;
    }
    return true;
}

bool ModAVDExtras::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;

    if (!handle_ || !touch_move_func_) {
        LogError << "ModAVD touch is not available";
        return false;
    }

    int ret = touch_move_func_(handle_, 0, contact, x, y, 0);
    if (ret != kOk) {
        LogError << "Failed to touch_move" << VAR(ret) << VAR(error_string(ret)) << VAR(contact) << VAR(x) << VAR(y);
        return false;
    }
    return true;
}

bool ModAVDExtras::touch_up(int contact)
{
    if (!handle_ || !touch_up_func_) {
        LogError << "ModAVD touch is not available";
        return false;
    }

    LogInfo << VAR(contact);

    int ret = touch_up_func_(handle_, 0, contact);
    if (ret != kOk) {
        LogError << "Failed to touch_up" << VAR(ret) << VAR(error_string(ret));
        return false;
    }
    return true;
}

bool ModAVDExtras::click_key(int key)
{
    LogInfo << VAR(key);

    if (handle_ && (capabilities_ & kCapKey)) {
        int ret = key_func_(handle_, 0, key, 1);
        if (ret == kOk) {
            ret = key_func_(handle_, 0, key, 0);
            if (ret == kOk) {
                return true;
            }
            // The key is down; do not send a second press through adb.
            LogError << "Failed to release key" << VAR(key) << VAR(ret) << VAR(error_string(ret));
            return false;
        }
        // MODAVD_E_UNSUPPORTED is about this key (one the emulator keyboard does not have), not about keys in general.
        LogWarn << "ModAVD key failed, fallback to adb" << VAR(key) << VAR(ret) << VAR(error_string(ret));
    }

    if (!adb_shell_input_) {
        LogError << "adb_shell_input_ is nullptr";
        return false;
    }
    return adb_shell_input_->click_key(key);
}

bool ModAVDExtras::key_down(int key)
{
    LogInfo << VAR(key);

    if (handle_ && (capabilities_ & kCapKey)) {
        int ret = key_func_(handle_, 0, key, 1);
        if (ret == kOk) {
            adb_clicked_keys_.erase(key);
            return true;
        }
        LogWarn << "ModAVD key_down failed, fallback to adb" << VAR(key) << VAR(ret) << VAR(error_string(ret));
    }

    // adb can only send a whole key press: send it now and make the matching key_up a no-op.
    if (!adb_shell_input_) {
        LogError << "adb_shell_input_ is nullptr";
        return false;
    }
    if (!adb_shell_input_->click_key(key)) {
        return false;
    }
    adb_clicked_keys_.emplace(key);
    return true;
}

bool ModAVDExtras::key_up(int key)
{
    LogInfo << VAR(key);

    if (adb_clicked_keys_.erase(key)) {
        return true;
    }

    if (!handle_ || !key_func_) {
        LogError << "ModAVD key is not available" << VAR(key);
        return false;
    }

    int ret = key_func_(handle_, 0, key, 0);
    if (ret != kOk) {
        LogError << "Failed to key_up" << VAR(key) << VAR(ret) << VAR(error_string(ret));
        return false;
    }
    return true;
}

bool ModAVDExtras::input_text(const std::string& text)
{
    LogInfo << VAR(text) << VAR(text.size());

    if (handle_ && (capabilities_ & kCapText)) {
        int ret = input_text_func_(handle_, text.c_str());
        if (ret == kOk) {
            return true;
        }
        // MODAVD_E_UNSUPPORTED is about this text (not ASCII, and no text field with ModAVD's input method focused),
        // so the next call tries the library again.
        LogWarn << "ModAVD input_text failed, fallback to adb" << VAR(ret) << VAR(error_string(ret));
    }

    if (!adb_shell_input_) {
        LogError << "adb_shell_input_ is nullptr";
        return false;
    }
    return adb_shell_input_->input_text(text);
}

void ModAVDExtras::on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur)
{
    std::ignore = pre;
    std::ignore = cur;

    if (!library_loaded_) {
        return;
    }
    reopen_instance();
}

bool ModAVDExtras::load_modavd_library()
{
    if (!Library::load_library(lib_path_)) {
        LogError << "Failed to load library" << VAR(lib_path_);
        return false;
    }
    library_loaded_ = true;

    api_version_func_ = Library::get_function<ApiVersionFunc>(kApiVersionFuncName);
    open_func_ = Library::get_function<OpenFunc>(kOpenFuncName);
    close_func_ = Library::get_function<CloseFunc>(kCloseFuncName);
    capabilities_func_ = Library::get_function<CapabilitiesFunc>(kCapabilitiesFuncName);
    capture_func_ = Library::get_function<CaptureFunc>(kCaptureFuncName);
    strerror_func_ = Library::get_function<StrErrorFunc>(kStrErrorFuncName);

    if (!api_version_func_ || !open_func_ || !close_func_ || !capabilities_func_ || !capture_func_) {
        LogError << "ModAVD extras library is missing functions" << VAR(lib_path_);
        unload_modavd_library();
        return false;
    }

    // API versions only grow: 1 capture, 2 touch, 3 keys and text. What an instance can do is announced through
    // MODAVD_CAP_*: text from version 3 on, keys when the emulator console has the key command.
    api_version_ = api_version_func_();
    LogInfo << "ModAVD extras" << VAR(lib_path_) << VAR(api_version_);
    if (api_version_ < 1) {
        LogError << "ModAVD extras library is too old" << VAR(api_version_);
        unload_modavd_library();
        return false;
    }

    if (usage_ == Usage::Input && api_version_ >= 2) {
        touch_down_func_ = Library::get_function<TouchFunc>(kTouchDownFuncName);
        touch_move_func_ = Library::get_function<TouchFunc>(kTouchMoveFuncName);
        touch_up_func_ = Library::get_function<TouchUpFunc>(kTouchUpFuncName);
        key_func_ = Library::get_function<KeyFunc>(kKeyFuncName);
        input_text_func_ = Library::get_function<InputTextFunc>(kInputTextFuncName);
    }

    return true;
}

void ModAVDExtras::unload_modavd_library()
{
    // The functions point into the library; drop them before it goes away.
    api_version_func_.clear();
    open_func_.clear();
    close_func_.clear();
    capabilities_func_.clear();
    capture_func_.clear();
    strerror_func_.clear();
    touch_down_func_.clear();
    touch_move_func_.clear();
    touch_up_func_.clear();
    key_func_.clear();
    input_text_func_.clear();
    api_version_ = 0;

    // Only give back the reference this unit took.
    if (library_loaded_) {
        Library::unload_library();
        library_loaded_ = false;
    }
}

bool ModAVDExtras::open_instance()
{
    if (!open_func_ || !capabilities_func_ || !close_func_) {
        return false;
    }

    OpenParams params { };
    params.size = sizeof(params);
    params.adb_address = address_.empty() ? nullptr : address_.c_str();
    params.console_port = console_port_ > 0 ? console_port_ : 0;
    params.client_name = "MaaFramework";

    void* handle = nullptr;
    int ret = open_func_(&params, &handle);
    if (ret != kOk || !handle) {
        LogError << "Failed to open ModAVD instance" << VAR(address_) << VAR(console_port_) << VAR(ret) << VAR(error_string(ret));
        return false;
    }

    handle_ = handle;
    capabilities_ = capabilities_func_(handle_);
    if (!key_func_) {
        capabilities_ &= ~kCapKey;
    }
    if (!input_text_func_) {
        capabilities_ &= ~kCapText;
    }
    LogInfo << "Opened ModAVD instance" << VAR(address_) << VAR(console_port_) << VAR(capabilities_);
    return true;
}

void ModAVDExtras::close_instance()
{
    // modavd_close lifts the contacts this handle still holds down
    if (handle_ && close_func_) {
        close_func_(handle_);
    }
    handle_ = nullptr;
    capabilities_ = 0;
    adb_clicked_keys_.clear();
}

bool ModAVDExtras::reopen_instance()
{
    LogFunc << VAR(address_) << VAR(console_port_);

    close_instance();
    return open_instance();
}

std::string ModAVDExtras::error_string(int ret) const
{
    if (!strerror_func_) {
        return { };
    }
    const char* s = strerror_func_(ret);
    return s ? s : "";
}

MAA_CTRL_UNIT_NS_END

#endif
