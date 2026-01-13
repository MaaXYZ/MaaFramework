#include "GamepadControlUnitMgr.h"

#include "ControlUnit/Win32ControlUnitAPI.h"
#include "Input/ViGEmInput.h"
#include "MaaUtils/LibraryHolder.h"
#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

// Win32ControlUnit loader - dynamically loads MaaWin32ControlUnit library
class Win32ControlUnitLoader : public LibraryHolder<Win32ControlUnitLoader>
{
public:
    Win32ControlUnitLoader() = default;

    ~Win32ControlUnitLoader() { destroy(); }

    bool load()
    {
#ifdef _WIN32
        if (!load_library("MaaWin32ControlUnit")) {
            LogError << "Failed to load MaaWin32ControlUnit library";
            return false;
        }

        create_func_ = get_function<decltype(::MaaWin32ControlUnitCreate)>("MaaWin32ControlUnitCreate");
        destroy_func_ = get_function<decltype(::MaaWin32ControlUnitDestroy)>("MaaWin32ControlUnitDestroy");

        if (!create_func_ || !destroy_func_) {
            LogError << "Failed to get Win32ControlUnit API functions";
            unload();
            return false;
        }

        return true;
#else
        return false;
#endif
    }

    void unload()
    {
        create_func_.clear();
        destroy_func_.clear();
        unload_library();
    }

    MaaWin32ControlUnitHandle create(void* hwnd, MaaWin32ScreencapMethod screencap_method)
    {
        if (!create_func_) {
            return nullptr;
        }

        handle_ = create_func_(hwnd, screencap_method, MaaWin32InputMethod_None, MaaWin32InputMethod_None);
        return handle_;
    }

    void destroy()
    {
        if (handle_ && destroy_func_) {
            destroy_func_(handle_);
            handle_ = nullptr;
        }
    }

    MaaWin32ControlUnitHandle handle() const { return handle_; }

private:
    boost::function<decltype(::MaaWin32ControlUnitCreate)> create_func_;
    boost::function<decltype(::MaaWin32ControlUnitDestroy)> destroy_func_;
    MaaWin32ControlUnitHandle handle_ = nullptr;
};

GamepadControlUnitMgr::GamepadControlUnitMgr(HWND hWnd, MaaGamepadType gamepad_type, MaaWin32ScreencapMethod screencap_method)
    : hwnd_(hWnd)
    , gamepad_type_(gamepad_type)
    , screencap_method_(screencap_method)
{
}

GamepadControlUnitMgr::~GamepadControlUnitMgr()
{
    if (gamepad_input_) {
        gamepad_input_->reset();
    }
}

bool GamepadControlUnitMgr::connect()
{
    connected_ = false;

    // Initialize gamepad input
    gamepad_input_ = std::make_unique<ViGEmInput>(gamepad_type_);
    if (!gamepad_input_->connect()) {
        LogError << "Failed to connect gamepad";
        return false;
    }

    // Initialize Win32 screencap if hwnd is provided
    if (hwnd_ && screencap_method_ != MaaWin32ScreencapMethod_None) {
        if (!init_win32_unit()) {
            LogWarn << "Failed to initialize Win32 screencap, screencap will not be available";
            // Don't fail the entire connection, gamepad input is still usable
        }
    }

    connected_ = true;
    LogInfo << "Gamepad controller connected, type:" << (gamepad_type_ == MaaGamepadType_Xbox360 ? "Xbox360" : "DualShock4");
    return true;
}

bool GamepadControlUnitMgr::init_win32_unit()
{
    // Dynamically load MaaWin32ControlUnit.dll
    win32_loader_ = std::make_unique<Win32ControlUnitLoader>();
    if (!win32_loader_->load()) {
        LogError << "Failed to load Win32ControlUnit library";
        win32_loader_.reset();
        return false;
    }

    auto handle = win32_loader_->create(hwnd_, screencap_method_);
    if (!handle) {
        LogError << "Failed to create Win32 control unit for screencap";
        win32_loader_.reset();
        return false;
    }

    win32_unit_ = std::shared_ptr<ControlUnitAPI>(handle, [](ControlUnitAPI*) {
        // Destructor handled by Win32ControlUnitLoader
    });

    if (!win32_unit_->connect()) {
        LogError << "Failed to connect Win32 control unit";
        win32_unit_.reset();
        win32_loader_.reset();
        return false;
    }

    LogInfo << "Win32 screencap initialized";
    return true;
}

void GamepadControlUnitMgr::ensure_foreground()
{
    if (!hwnd_) {
        return;
    }

    if (hwnd_ == GetForegroundWindow()) {
        return;
    }

    SetForegroundWindow(hwnd_);
}

bool GamepadControlUnitMgr::connected() const
{
    return connected_ && gamepad_input_ && gamepad_input_->connected();
}

bool GamepadControlUnitMgr::request_uuid(std::string& uuid)
{
    std::string_view type_str = (gamepad_type_ == MaaGamepadType_Xbox360 ? "xbox360" : "ds4");
    uuid = std::format("gamepad_{}_hwnd_{}", type_str, reinterpret_cast<uintptr_t>(hwnd_));
    return true;
}

MaaControllerFeature GamepadControlUnitMgr::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick
           | MaaControllerFeature_NoScalingTouchPoints;
}

bool GamepadControlUnitMgr::start_app(const std::string& intent)
{
    if (!win32_unit_) {
        LogError << "Win32 screencap not available (hwnd not provided or init failed)";
        return false;
    }

    return win32_unit_->start_app(intent);
}

bool GamepadControlUnitMgr::stop_app(const std::string& intent)
{
    if (!win32_unit_) {
        LogError << "Win32 screencap not available (hwnd not provided or init failed)";
        return false;
    }

    return win32_unit_->stop_app(intent);
}

bool GamepadControlUnitMgr::screencap(cv::Mat& image)
{
    if (!win32_unit_) {
        LogError << "Win32 screencap not available (hwnd not provided or init failed)";
        return false;
    }

    return win32_unit_->screencap(image);
}

// get_features() 返回 MaaControllerFeature_UseMouseDownAndUpInsteadOfClick，
// 上层 ControllerAgent 会使用 touch_down/touch_up 替代 click/swipe
bool GamepadControlUnitMgr::click(int x, int y)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_up instead"
             << VAR(x) << VAR(y);
    return false;
}

bool GamepadControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_move/touch_up instead"
             << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool GamepadControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    if (!gamepad_input_ || !gamepad_input_->connected()) {
        LogError << "Gamepad not connected";
        return false;
    }

    ensure_foreground();

    switch (contact) {
    case MaaGamepadTouch_LeftStick:
        return gamepad_input_->set_left_stick(x, y);
    case MaaGamepadTouch_RightStick:
        return gamepad_input_->set_right_stick(x, y);
    case MaaGamepadTouch_LeftTrigger:
        return gamepad_input_->set_left_trigger(pressure);
    case MaaGamepadTouch_RightTrigger:
        return gamepad_input_->set_right_trigger(pressure);
    default:
        LogError << "Invalid gamepad touch contact:" << contact;
        return false;
    }
}

bool GamepadControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    // Same as touch_down for gamepad
    return touch_down(contact, x, y, pressure);
}

bool GamepadControlUnitMgr::touch_up(int contact)
{
    if (!gamepad_input_ || !gamepad_input_->connected()) {
        LogError << "Gamepad not connected";
        return false;
    }

    switch (contact) {
    case MaaGamepadTouch_LeftStick:
        return gamepad_input_->reset_left_stick();
    case MaaGamepadTouch_RightStick:
        return gamepad_input_->reset_right_stick();
    case MaaGamepadTouch_LeftTrigger:
        return gamepad_input_->reset_left_trigger();
    case MaaGamepadTouch_RightTrigger:
        return gamepad_input_->reset_right_trigger();
    default:
        LogError << "Invalid gamepad touch contact:" << contact;
        return false;
    }
}

// get_features() 返回 MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick，
// 上层 ControllerAgent 会使用 key_down/key_up 替代 click_key
bool GamepadControlUnitMgr::click_key(int key)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick, "
                "use key_down/key_up instead"
             << VAR(key);
    return false;
}

bool GamepadControlUnitMgr::input_text(const std::string& text)
{
    LogError << "input_text not supported for gamepad controller" << VAR(text);
    return false;
}

bool GamepadControlUnitMgr::key_down(int key)
{
    if (!gamepad_input_ || !gamepad_input_->connected()) {
        LogError << "Gamepad not connected";
        return false;
    }

    ensure_foreground();

    return gamepad_input_->press_button(key);
}

bool GamepadControlUnitMgr::key_up(int key)
{
    if (!gamepad_input_ || !gamepad_input_->connected()) {
        LogError << "Gamepad not connected";
        return false;
    }

    return gamepad_input_->release_button(key);
}

bool GamepadControlUnitMgr::scroll(int dx, int dy)
{
    LogError << "scroll not supported for gamepad controller" << VAR(dx) << VAR(dy);
    return false;
}

MAA_CTRL_UNIT_NS_END
