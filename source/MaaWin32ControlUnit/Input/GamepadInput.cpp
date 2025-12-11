#include "GamepadInput.h"

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

GamepadInput::GamepadInput()
{
    if (load_vigem()) {
        inited_ = init_gamepad();
    }
}

GamepadInput::~GamepadInput()
{
    uninit_gamepad();
    unload_vigem();
}

bool GamepadInput::load_vigem()
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

void GamepadInput::unload_vigem()
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

bool GamepadInput::init_gamepad()
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

void GamepadInput::uninit_gamepad()
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
    inited_ = false;
}

MaaControllerFeature GamepadInput::get_features() const
{
    return MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool GamepadInput::click(int x, int y)
{
    std::ignore = x;
    std::ignore = y;
    LogWarn << "Gamepad does not support click";
    return false;
}

bool GamepadInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    std::ignore = x1;
    std::ignore = y1;
    std::ignore = x2;
    std::ignore = y2;
    std::ignore = duration;
    LogWarn << "Gamepad does not support swipe";
    return false;
}

bool GamepadInput::touch_down(int contact, int x, int y, int pressure)
{
    std::ignore = contact;
    std::ignore = x;
    std::ignore = y;
    std::ignore = pressure;
    LogWarn << "Gamepad does not support touch_down";
    return false;
}

bool GamepadInput::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = contact;
    std::ignore = x;
    std::ignore = y;
    std::ignore = pressure;
    LogWarn << "Gamepad does not support touch_move";
    return false;
}

bool GamepadInput::touch_up(int contact)
{
    std::ignore = contact;
    LogWarn << "Gamepad does not support touch_up";
    return false;
}

bool GamepadInput::click_key(int key)
{
    LogError << "deprecated" << VAR(key);
    return false;
}

bool GamepadInput::input_text(const std::string& text)
{
    std::ignore = text;
    LogWarn << "Gamepad does not support input_text";
    return false;
}

bool GamepadInput::key_down(int key)
{
    LogInfo << VAR(key);
    return set_button(static_cast<WORD>(key), true);
}

bool GamepadInput::key_up(int key)
{
    LogInfo << VAR(key);
    return set_button(static_cast<WORD>(key), false);
}

bool GamepadInput::scroll(int dx, int dy)
{
    std::ignore = dx;
    std::ignore = dy;
    LogWarn << "Gamepad does not support scroll";
    return false;
}

bool GamepadInput::set_button(WORD button, bool pressed)
{
    if (!inited_) {
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

bool GamepadInput::set_left_stick(SHORT x, SHORT y)
{
    if (!inited_) {
        LogError << "Gamepad not initialized";
        return false;
    }

    LogInfo << VAR(x) << VAR(y);

    report_.sThumbLX = x;
    report_.sThumbLY = y;

    return send_state();
}

bool GamepadInput::set_right_stick(SHORT x, SHORT y)
{
    if (!inited_) {
        LogError << "Gamepad not initialized";
        return false;
    }

    LogInfo << VAR(x) << VAR(y);

    report_.sThumbRX = x;
    report_.sThumbRY = y;

    return send_state();
}

bool GamepadInput::set_left_trigger(BYTE value)
{
    if (!inited_) {
        LogError << "Gamepad not initialized";
        return false;
    }

    LogInfo << VAR(value);

    report_.bLeftTrigger = value;

    return send_state();
}

bool GamepadInput::set_right_trigger(BYTE value)
{
    if (!inited_) {
        LogError << "Gamepad not initialized";
        return false;
    }

    LogInfo << VAR(value);

    report_.bRightTrigger = value;

    return send_state();
}

bool GamepadInput::send_state()
{
    if (!inited_ || !client_ || !pad_ || !fn_target_x360_update_) {
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
