#include "GamepadControlUnitMgr.h"

#include <algorithm>

#include "MaaUtils/Logger.h"
#include "MaaUtils/Runtime.h"

MAA_CTRL_UNIT_NS_BEGIN

std::shared_ptr<Win32ControlUnitAPI> Win32ScreencapLibraryHolder::create_control_unit(void* hWnd, MaaWin32ScreencapMethod screencap_method)
{
    static const std::filesystem::path libname = MAA_NS::path("MaaWin32ControlUnit");
    static const std::string version_func_name = "MaaWin32ControlUnitGetVersion";
    static const std::string create_func_name = "MaaWin32ControlUnitCreate";
    static const std::string destroy_func_name = "MaaWin32ControlUnitDestroy";

    if (!load_library(library_dir() / libname)) {
        LogError << "Failed to load library" << VAR(library_dir()) << VAR(libname);
        return nullptr;
    }

    auto version_func = get_function<decltype(MaaWin32ControlUnitGetVersion)>(version_func_name);
    if (version_func) {
        LogInfo << "Win32ControlUnit version:" << version_func();
    }

    auto create_func = get_function<decltype(MaaWin32ControlUnitCreate)>(create_func_name);
    if (!create_func) {
        LogError << "Failed to get function" << create_func_name;
        return nullptr;
    }

    auto destroy_func = get_function<decltype(MaaWin32ControlUnitDestroy)>(destroy_func_name);
    if (!destroy_func) {
        LogError << "Failed to get function" << destroy_func_name;
        return nullptr;
    }

    // 创建 Win32 控制单元，输入方式设为 None（我们只用截图功能）
    auto handle = create_func(hWnd, screencap_method, MaaWin32InputMethod_None, MaaWin32InputMethod_None);
    if (!handle) {
        LogError << "Failed to create Win32 control unit";
        return nullptr;
    }

    return std::shared_ptr<Win32ControlUnitAPI>(handle, destroy_func);
}

GamepadControlUnitMgr::GamepadControlUnitMgr(HWND hWnd, MaaWin32ScreencapMethod screencap_method)
    : hwnd_(hWnd)
    , screencap_method_(screencap_method)
{
    LogFunc << VAR_VOIDP(hWnd) << VAR(screencap_method);
}

GamepadControlUnitMgr::~GamepadControlUnitMgr()
{
    LogFunc;
    uninit_gamepad();
    unload_vigem();
}

bool GamepadControlUnitMgr::connect()
{
    LogFunc;

    // 初始化 Win32 截图
    if (screencap_method_ != MaaWin32ScreencapMethod_None) {
        win32_unit_ = Win32ScreencapLibraryHolder::create_control_unit(hwnd_, screencap_method_);
        if (!win32_unit_) {
            LogError << "Failed to create Win32 control unit for screencap";
            return false;
        }
        if (!win32_unit_->connect()) {
            LogError << "Failed to connect Win32 control unit";
            return false;
        }
    }

    // 初始化 ViGEm 手柄
    if (!load_vigem()) {
        return false;
    }
    gamepad_inited_ = init_gamepad();
    return gamepad_inited_;
}

bool GamepadControlUnitMgr::connected() const
{
    return gamepad_inited_ && win32_unit_ && win32_unit_->connected();
}

bool GamepadControlUnitMgr::request_uuid(std::string& uuid)
{
    std::stringstream ss;
    ss << "ViGEm-Gamepad-" << hwnd_;
    uuid = std::move(ss).str();
    return true;
}

bool GamepadControlUnitMgr::load_vigem()
{
    vigem_module_ = LoadLibraryW(L"ViGEmClient.dll");
    if (!vigem_module_) {
        LogError << "Failed to load ViGEmClient.dll. Please install ViGEmBus driver.";
        return false;
    }

    fn_alloc_ = reinterpret_cast<vigem::PFN_vigem_alloc>(GetProcAddress(vigem_module_, "vigem_alloc"));
    fn_free_ = reinterpret_cast<vigem::PFN_vigem_free>(GetProcAddress(vigem_module_, "vigem_free"));
    fn_connect_ = reinterpret_cast<vigem::PFN_vigem_connect>(GetProcAddress(vigem_module_, "vigem_connect"));
    fn_disconnect_ = reinterpret_cast<vigem::PFN_vigem_disconnect>(GetProcAddress(vigem_module_, "vigem_disconnect"));
    fn_target_x360_alloc_ = reinterpret_cast<vigem::PFN_vigem_target_x360_alloc>(GetProcAddress(vigem_module_, "vigem_target_x360_alloc"));
    fn_target_free_ = reinterpret_cast<vigem::PFN_vigem_target_free>(GetProcAddress(vigem_module_, "vigem_target_free"));
    fn_target_add_ = reinterpret_cast<vigem::PFN_vigem_target_add>(GetProcAddress(vigem_module_, "vigem_target_add"));
    fn_target_remove_ = reinterpret_cast<vigem::PFN_vigem_target_remove>(GetProcAddress(vigem_module_, "vigem_target_remove"));
    fn_target_x360_update_ =
        reinterpret_cast<vigem::PFN_vigem_target_x360_update>(GetProcAddress(vigem_module_, "vigem_target_x360_update"));

    if (!fn_alloc_ || !fn_free_ || !fn_connect_ || !fn_disconnect_ || !fn_target_x360_alloc_ || !fn_target_free_ || !fn_target_add_
        || !fn_target_remove_ || !fn_target_x360_update_) {
        LogError << "Failed to get ViGEmClient function pointers";
        FreeLibrary(vigem_module_);
        vigem_module_ = nullptr;
        return false;
    }

    LogInfo << "ViGEmClient.dll loaded successfully";
    return true;
}

void GamepadControlUnitMgr::unload_vigem()
{
    if (vigem_module_) {
        FreeLibrary(vigem_module_);
        vigem_module_ = nullptr;
    }
    fn_alloc_ = nullptr;
    fn_free_ = nullptr;
    fn_connect_ = nullptr;
    fn_disconnect_ = nullptr;
    fn_target_x360_alloc_ = nullptr;
    fn_target_free_ = nullptr;
    fn_target_add_ = nullptr;
    fn_target_remove_ = nullptr;
    fn_target_x360_update_ = nullptr;
}

bool GamepadControlUnitMgr::init_gamepad()
{
    if (!fn_alloc_) {
        return false;
    }

    client_ = fn_alloc_();
    if (!client_) {
        LogError << "vigem_alloc failed";
        return false;
    }

    vigem::VIGEM_ERROR ret = fn_connect_(client_);
    if (!vigem::VIGEM_SUCCESS(ret)) {
        LogError << "vigem_connect failed, please install ViGEmBus driver" << VAR(static_cast<int>(ret));
        fn_free_(client_);
        client_ = nullptr;
        return false;
    }

    pad_ = fn_target_x360_alloc_();
    if (!pad_) {
        LogError << "vigem_target_x360_alloc failed";
        fn_disconnect_(client_);
        fn_free_(client_);
        client_ = nullptr;
        return false;
    }

    ret = fn_target_add_(client_, pad_);
    if (!vigem::VIGEM_SUCCESS(ret)) {
        LogError << "vigem_target_add failed" << VAR(static_cast<int>(ret));
        fn_target_free_(pad_);
        pad_ = nullptr;
        fn_disconnect_(client_);
        fn_free_(client_);
        client_ = nullptr;
        return false;
    }

    LogInfo << "ViGEm virtual Xbox 360 gamepad initialized";
    return true;
}

void GamepadControlUnitMgr::uninit_gamepad()
{
    if (pad_ && client_ && fn_target_remove_ && fn_target_free_) {
        fn_target_remove_(client_, pad_);
        fn_target_free_(pad_);
        pad_ = nullptr;
    }
    if (client_ && fn_disconnect_ && fn_free_) {
        fn_disconnect_(client_);
        fn_free_(client_);
        client_ = nullptr;
    }
    gamepad_inited_ = false;
}

MaaControllerFeature GamepadControlUnitMgr::get_features() const
{
    return MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool GamepadControlUnitMgr::start_app(const std::string& intent)
{
    std::ignore = intent;
    LogWarn << "Gamepad does not support start_app";
    return false;
}

bool GamepadControlUnitMgr::stop_app(const std::string& intent)
{
    std::ignore = intent;
    LogWarn << "Gamepad does not support stop_app";
    return false;
}

bool GamepadControlUnitMgr::screencap(cv::Mat& image)
{
    if (!win32_unit_) {
        LogError << "Win32 control unit not initialized";
        return false;
    }
    return win32_unit_->screencap(image);
}

bool GamepadControlUnitMgr::click(int x, int y)
{
    std::ignore = x;
    std::ignore = y;
    LogWarn << "Gamepad does not support click";
    return false;
}

bool GamepadControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    std::ignore = x1;
    std::ignore = y1;
    std::ignore = x2;
    std::ignore = y2;
    std::ignore = duration;
    LogWarn << "Gamepad does not support swipe";
    return false;
}

bool GamepadControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    switch (contact) {
    case 0:
        return set_left_stick(static_cast<SHORT>(x), static_cast<SHORT>(y));
    case 1:
        return set_right_stick(static_cast<SHORT>(x), static_cast<SHORT>(y));
    case 2:
        return set_left_trigger(static_cast<BYTE>(std::clamp(pressure, 0, 255)));
    case 3:
        return set_right_trigger(static_cast<BYTE>(std::clamp(pressure, 0, 255)));
    default:
        LogWarn << "Gamepad only supports contact 0-3 (LS/RS/LT/RT)";
        return false;
    }
}

bool GamepadControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    return touch_down(contact, x, y, pressure);
}

bool GamepadControlUnitMgr::touch_up(int contact)
{
    LogInfo << VAR(contact);

    switch (contact) {
    case 0:
        return set_left_stick(0, 0);
    case 1:
        return set_right_stick(0, 0);
    case 2:
        return set_left_trigger(0);
    case 3:
        return set_right_trigger(0);
    default:
        LogWarn << "Gamepad only supports contact 0-3 (LS/RS/LT/RT)";
        return false;
    }
}

bool GamepadControlUnitMgr::click_key(int key)
{
    LogError << "deprecated" << VAR(key);
    return false;
}

bool GamepadControlUnitMgr::input_text(const std::string& text)
{
    std::ignore = text;
    LogWarn << "Gamepad does not support input_text";
    return false;
}

bool GamepadControlUnitMgr::key_down(int key)
{
    LogInfo << VAR(key);

    return set_button(static_cast<WORD>(key), true);
}

bool GamepadControlUnitMgr::key_up(int key)
{
    LogInfo << VAR(key);

    return set_button(static_cast<WORD>(key), false);
}

bool GamepadControlUnitMgr::scroll(int dx, int dy)
{
    std::ignore = dx;
    std::ignore = dy;
    LogWarn << "Gamepad does not support scroll";
    return false;
}

bool GamepadControlUnitMgr::set_button(WORD button, bool pressed)
{
    if (!gamepad_inited_) {
        LogError << "Gamepad not initialized";
        return false;
    }

    LogInfo << VAR(button) << VAR(pressed);

    if (pressed) {
        report_.wButtons |= button;
    }
    else {
        report_.wButtons &= ~button;
    }

    return send_state();
}

bool GamepadControlUnitMgr::set_left_stick(SHORT x, SHORT y)
{
    if (!gamepad_inited_) {
        LogError << "Gamepad not initialized";
        return false;
    }

    LogInfo << VAR(x) << VAR(y);

    report_.sThumbLX = x;
    report_.sThumbLY = y;

    return send_state();
}

bool GamepadControlUnitMgr::set_right_stick(SHORT x, SHORT y)
{
    if (!gamepad_inited_) {
        LogError << "Gamepad not initialized";
        return false;
    }

    LogInfo << VAR(x) << VAR(y);

    report_.sThumbRX = x;
    report_.sThumbRY = y;

    return send_state();
}

bool GamepadControlUnitMgr::set_left_trigger(BYTE value)
{
    if (!gamepad_inited_) {
        LogError << "Gamepad not initialized";
        return false;
    }

    LogInfo << VAR(value);

    report_.bLeftTrigger = value;

    return send_state();
}

bool GamepadControlUnitMgr::set_right_trigger(BYTE value)
{
    if (!gamepad_inited_) {
        LogError << "Gamepad not initialized";
        return false;
    }

    LogInfo << VAR(value);

    report_.bRightTrigger = value;

    return send_state();
}

bool GamepadControlUnitMgr::send_state()
{
    if (!gamepad_inited_ || !client_ || !pad_ || !fn_target_x360_update_) {
        LogError << "Gamepad not initialized";
        return false;
    }

    vigem::VIGEM_ERROR ret = fn_target_x360_update_(client_, pad_, report_);
    if (!vigem::VIGEM_SUCCESS(ret)) {
        LogError << "vigem_target_x360_update failed" << VAR(static_cast<int>(ret));
        return false;
    }

    return true;
}

MAA_CTRL_UNIT_NS_END
