#include "ViGEmInput.h"

#include "MaaUtils/Logger.h"

// ViGEm types and constants (copied from headers to avoid include dependency)
namespace
{
constexpr uint32_t VIGEM_ERROR_NONE = 0x20000000;

// Xbox button flags (same as XUSB_BUTTON)
constexpr uint16_t XBOX_DPAD_UP = 0x0001;
constexpr uint16_t XBOX_DPAD_DOWN = 0x0002;
constexpr uint16_t XBOX_DPAD_LEFT = 0x0004;
constexpr uint16_t XBOX_DPAD_RIGHT = 0x0008;
constexpr uint16_t XBOX_START = 0x0010;
constexpr uint16_t XBOX_BACK = 0x0020;
constexpr uint16_t XBOX_LEFT_THUMB = 0x0040;
constexpr uint16_t XBOX_RIGHT_THUMB = 0x0080;
constexpr uint16_t XBOX_LEFT_SHOULDER = 0x0100;
constexpr uint16_t XBOX_RIGHT_SHOULDER = 0x0200;
constexpr uint16_t XBOX_GUIDE = 0x0400;
constexpr uint16_t XBOX_A = 0x1000;
constexpr uint16_t XBOX_B = 0x2000;
constexpr uint16_t XBOX_X = 0x4000;
constexpr uint16_t XBOX_Y = 0x8000;

// DS4 button flags
constexpr uint16_t DS4_SQUARE = 0x0010;
constexpr uint16_t DS4_CROSS = 0x0020;
constexpr uint16_t DS4_CIRCLE = 0x0040;
constexpr uint16_t DS4_TRIANGLE = 0x0080;
constexpr uint16_t DS4_L1 = 0x0100;
constexpr uint16_t DS4_R1 = 0x0200;
constexpr uint16_t DS4_L2 = 0x0400; // trigger button
constexpr uint16_t DS4_R2 = 0x0800; // trigger button
constexpr uint16_t DS4_SHARE = 0x1000;
constexpr uint16_t DS4_OPTIONS = 0x2000;
constexpr uint16_t DS4_L_THUMB = 0x4000;
constexpr uint16_t DS4_R_THUMB = 0x8000;

// DS4 special buttons
constexpr uint8_t DS4_SPECIAL_PS = 0x01;
constexpr uint8_t DS4_SPECIAL_TOUCHPAD = 0x02;

// DS4 DPAD values
constexpr uint16_t DS4_DPAD_NONE = 0x8;

uint16_t convert_xbox_to_ds4_button(int button)
{
    // Convert Xbox button codes to DS4 button codes
    uint16_t ds4_button = 0;

    if (button & XBOX_A) {
        ds4_button |= DS4_CROSS;
    }
    if (button & XBOX_B) {
        ds4_button |= DS4_CIRCLE;
    }
    if (button & XBOX_X) {
        ds4_button |= DS4_SQUARE;
    }
    if (button & XBOX_Y) {
        ds4_button |= DS4_TRIANGLE;
    }
    if (button & XBOX_LEFT_SHOULDER) {
        ds4_button |= DS4_L1;
    }
    if (button & XBOX_RIGHT_SHOULDER) {
        ds4_button |= DS4_R1;
    }
    if (button & XBOX_LEFT_THUMB) {
        ds4_button |= DS4_L_THUMB;
    }
    if (button & XBOX_RIGHT_THUMB) {
        ds4_button |= DS4_R_THUMB;
    }
    if (button & XBOX_START) {
        ds4_button |= DS4_OPTIONS;
    }
    if (button & XBOX_BACK) {
        ds4_button |= DS4_SHARE;
    }

    return ds4_button;
}

uint8_t convert_xbox_to_ds4_special(int button)
{
    uint8_t special = 0;
    if (button & XBOX_GUIDE) {
        special |= DS4_SPECIAL_PS;
    }
    return special;
}
} // namespace

MAA_CTRL_UNIT_NS_BEGIN

ViGEmInput::ViGEmInput(MaaGamepadType type)
    : type_(type)
{
    // Initialize state to default
    if (type_ == MaaGamepadType_Xbox360) {
        state_.xbox = {};
    }
    else {
        state_.ds4 = {};
        state_.ds4.bThumbLX = 0x80; // center position
        state_.ds4.bThumbLY = 0x80;
        state_.ds4.bThumbRX = 0x80;
        state_.ds4.bThumbRY = 0x80;
        state_.ds4.wButtons = DS4_DPAD_NONE;
    }
}

ViGEmInput::~ViGEmInput()
{
    cleanup_vigem();
    unload_vigem_library();
}

bool ViGEmInput::load_vigem_library()
{
    if (!load_library("ViGEmClient")) {
        LogError << "Failed to load ViGEmClient library";
        return false;
    }

    fn_vigem_alloc_ = get_function<FnVigemAlloc>("vigem_alloc");
    fn_vigem_free_ = get_function<FnVigemFree>("vigem_free");
    fn_vigem_connect_ = get_function<FnVigemConnect>("vigem_connect");
    fn_vigem_disconnect_ = get_function<FnVigemDisconnect>("vigem_disconnect");
    fn_vigem_target_x360_alloc_ = get_function<FnVigemTargetAlloc>("vigem_target_x360_alloc");
    fn_vigem_target_ds4_alloc_ = get_function<FnVigemTargetAlloc>("vigem_target_ds4_alloc");
    fn_vigem_target_free_ = get_function<FnVigemTargetFree>("vigem_target_free");
    fn_vigem_target_add_ = get_function<FnVigemTargetAdd>("vigem_target_add");
    fn_vigem_target_remove_ = get_function<FnVigemTargetRemove>("vigem_target_remove");
    fn_vigem_target_x360_update_ = get_function<FnVigemTargetX360Update>("vigem_target_x360_update");
    fn_vigem_target_ds4_update_ = get_function<FnVigemTargetDs4Update>("vigem_target_ds4_update");

    if (!fn_vigem_alloc_ || !fn_vigem_free_ || !fn_vigem_connect_ || !fn_vigem_disconnect_ || !fn_vigem_target_free_
        || !fn_vigem_target_add_ || !fn_vigem_target_remove_) {
        LogError << "Failed to get ViGEm function pointers";
        unload_vigem_library();
        return false;
    }

    if (type_ == MaaGamepadType_Xbox360) {
        if (!fn_vigem_target_x360_alloc_ || !fn_vigem_target_x360_update_) {
            LogError << "Failed to get Xbox 360 function pointers";
            unload_vigem_library();
            return false;
        }
    }
    else {
        if (!fn_vigem_target_ds4_alloc_ || !fn_vigem_target_ds4_update_) {
            LogError << "Failed to get DS4 function pointers";
            unload_vigem_library();
            return false;
        }
    }

    LogInfo << "ViGEmClient library loaded successfully";
    return true;
}

void ViGEmInput::unload_vigem_library()
{
    fn_vigem_alloc_ = nullptr;
    fn_vigem_free_ = nullptr;
    fn_vigem_connect_ = nullptr;
    fn_vigem_disconnect_ = nullptr;
    fn_vigem_target_x360_alloc_ = nullptr;
    fn_vigem_target_ds4_alloc_ = nullptr;
    fn_vigem_target_free_ = nullptr;
    fn_vigem_target_add_ = nullptr;
    fn_vigem_target_remove_ = nullptr;
    fn_vigem_target_x360_update_ = nullptr;
    fn_vigem_target_ds4_update_ = nullptr;

    unload_library();
}

bool ViGEmInput::init_vigem()
{
    if (!load_vigem_library()) {
        return false;
    }

    vigem_client_ = fn_vigem_alloc_();
    if (!vigem_client_) {
        LogError << "Failed to allocate ViGEm client";
        return false;
    }

    auto ret = fn_vigem_connect_(vigem_client_);
    if (ret != VIGEM_ERROR_NONE) {
        LogError << "Failed to connect to ViGEm bus, error:" << std::hex << ret;
        cleanup_vigem();
        return false;
    }

    if (type_ == MaaGamepadType_Xbox360) {
        vigem_target_ = fn_vigem_target_x360_alloc_();
    }
    else {
        vigem_target_ = fn_vigem_target_ds4_alloc_();
    }

    if (!vigem_target_) {
        LogError << "Failed to allocate ViGEm target";
        cleanup_vigem();
        return false;
    }

    ret = fn_vigem_target_add_(vigem_client_, vigem_target_);
    if (ret != VIGEM_ERROR_NONE) {
        LogError << "Failed to add ViGEm target, error:" << std::hex << ret;
        cleanup_vigem();
        return false;
    }

    LogInfo << "ViGEm gamepad connected, type:" << (type_ == MaaGamepadType_Xbox360 ? "Xbox360" : "DualShock4");
    return true;
}

void ViGEmInput::cleanup_vigem()
{
    if (vigem_target_ && vigem_client_) {
        if (fn_vigem_target_remove_) {
            fn_vigem_target_remove_(vigem_client_, vigem_target_);
        }
    }

    if (vigem_target_) {
        if (fn_vigem_target_free_) {
            fn_vigem_target_free_(vigem_target_);
        }
        vigem_target_ = nullptr;
    }

    if (vigem_client_) {
        if (fn_vigem_disconnect_) {
            fn_vigem_disconnect_(vigem_client_);
        }
        if (fn_vigem_free_) {
            fn_vigem_free_(vigem_client_);
        }
        vigem_client_ = nullptr;
    }
}

bool ViGEmInput::connect()
{
    if (connected_) {
        return true;
    }

    connected_ = init_vigem();
    return connected_;
}

bool ViGEmInput::update_report()
{
    if (!connected_ || !vigem_client_ || !vigem_target_) {
        return false;
    }

    uint32_t ret = 0;
    if (type_ == MaaGamepadType_Xbox360) {
        XusbReport report;
        report.wButtons = state_.xbox.wButtons;
        report.bLeftTrigger = state_.xbox.bLeftTrigger;
        report.bRightTrigger = state_.xbox.bRightTrigger;
        report.sThumbLX = state_.xbox.sThumbLX;
        report.sThumbLY = state_.xbox.sThumbLY;
        report.sThumbRX = state_.xbox.sThumbRX;
        report.sThumbRY = state_.xbox.sThumbRY;
        ret = fn_vigem_target_x360_update_(vigem_client_, vigem_target_, report);
    }
    else {
        Ds4Report report;
        report.bThumbLX = state_.ds4.bThumbLX;
        report.bThumbLY = state_.ds4.bThumbLY;
        report.bThumbRX = state_.ds4.bThumbRX;
        report.bThumbRY = state_.ds4.bThumbRY;
        report.wButtons = state_.ds4.wButtons;
        report.bSpecial = state_.ds4.bSpecial;
        report.bTriggerL = state_.ds4.bTriggerL;
        report.bTriggerR = state_.ds4.bTriggerR;
        ret = fn_vigem_target_ds4_update_(vigem_client_, vigem_target_, report);
    }

    if (ret != VIGEM_ERROR_NONE) {
        LogError << "Failed to update ViGEm report, error:" << std::hex << ret;
        return false;
    }

    return true;
}

bool ViGEmInput::press_button(int button)
{
    if (!connected_) {
        return false;
    }

    if (type_ == MaaGamepadType_Xbox360) {
        // Xbox 360 uses the button values directly (lower 16 bits only)
        state_.xbox.wButtons |= static_cast<uint16_t>(button & 0xFFFF);
    }
    else {
        // Convert Xbox button values to DS4 protocol values
        uint16_t ds4_button = convert_xbox_to_ds4_button(button);
        uint8_t ds4_special = convert_xbox_to_ds4_special(button);

        // Check DS4-specific buttons (high bits)
        if (button & MaaGamepadButton_PS) {
            ds4_special |= DS4_SPECIAL_PS;
        }
        if (button & MaaGamepadButton_TOUCHPAD) {
            ds4_special |= DS4_SPECIAL_TOUCHPAD;
        }

        state_.ds4.wButtons |= ds4_button;
        state_.ds4.bSpecial |= ds4_special;
    }

    return update_report();
}

bool ViGEmInput::release_button(int button)
{
    if (!connected_) {
        return false;
    }

    if (type_ == MaaGamepadType_Xbox360) {
        state_.xbox.wButtons &= ~static_cast<uint16_t>(button & 0xFFFF);
    }
    else {
        uint16_t ds4_button = convert_xbox_to_ds4_button(button);
        uint8_t ds4_special = convert_xbox_to_ds4_special(button);

        if (button & MaaGamepadButton_PS) {
            ds4_special |= DS4_SPECIAL_PS;
        }
        if (button & MaaGamepadButton_TOUCHPAD) {
            ds4_special |= DS4_SPECIAL_TOUCHPAD;
        }

        state_.ds4.wButtons &= ~ds4_button;
        state_.ds4.bSpecial &= ~ds4_special;
    }

    return update_report();
}

bool ViGEmInput::click_button(int button)
{
    if (!press_button(button)) {
        return false;
    }
    Sleep(50); // Default press duration
    return release_button(button);
}

bool ViGEmInput::set_left_stick(int x, int y)
{
    if (!connected_) {
        return false;
    }

    if (type_ == MaaGamepadType_Xbox360) {
        state_.xbox.sThumbLX = static_cast<int16_t>(std::clamp(x, -32768, 32767));
        state_.xbox.sThumbLY = static_cast<int16_t>(std::clamp(y, -32768, 32767));
    }
    else {
        // DS4 uses 0-255 range with 128 as center
        state_.ds4.bThumbLX = static_cast<uint8_t>(std::clamp((x + 32768) * 255 / 65535, 0, 255));
        state_.ds4.bThumbLY = static_cast<uint8_t>(std::clamp((y + 32768) * 255 / 65535, 0, 255));
    }

    return update_report();
}

bool ViGEmInput::set_right_stick(int x, int y)
{
    if (!connected_) {
        return false;
    }

    if (type_ == MaaGamepadType_Xbox360) {
        state_.xbox.sThumbRX = static_cast<int16_t>(std::clamp(x, -32768, 32767));
        state_.xbox.sThumbRY = static_cast<int16_t>(std::clamp(y, -32768, 32767));
    }
    else {
        state_.ds4.bThumbRX = static_cast<uint8_t>(std::clamp((x + 32768) * 255 / 65535, 0, 255));
        state_.ds4.bThumbRY = static_cast<uint8_t>(std::clamp((y + 32768) * 255 / 65535, 0, 255));
    }

    return update_report();
}

bool ViGEmInput::reset_left_stick()
{
    return set_left_stick(0, 0);
}

bool ViGEmInput::reset_right_stick()
{
    return set_right_stick(0, 0);
}

bool ViGEmInput::set_left_trigger(int value)
{
    if (!connected_) {
        return false;
    }

    uint8_t clamped = static_cast<uint8_t>(std::clamp(value, 0, 255));

    if (type_ == MaaGamepadType_Xbox360) {
        state_.xbox.bLeftTrigger = clamped;
    }
    else {
        state_.ds4.bTriggerL = clamped;
    }

    return update_report();
}

bool ViGEmInput::set_right_trigger(int value)
{
    if (!connected_) {
        return false;
    }

    uint8_t clamped = static_cast<uint8_t>(std::clamp(value, 0, 255));

    if (type_ == MaaGamepadType_Xbox360) {
        state_.xbox.bRightTrigger = clamped;
    }
    else {
        state_.ds4.bTriggerR = clamped;
    }

    return update_report();
}

bool ViGEmInput::reset_left_trigger()
{
    return set_left_trigger(0);
}

bool ViGEmInput::reset_right_trigger()
{
    return set_right_trigger(0);
}

void ViGEmInput::reset()
{
    if (type_ == MaaGamepadType_Xbox360) {
        state_.xbox = {};
    }
    else {
        state_.ds4 = {};
        state_.ds4.bThumbLX = 0x80;
        state_.ds4.bThumbLY = 0x80;
        state_.ds4.bThumbRX = 0x80;
        state_.ds4.bThumbRY = 0x80;
        state_.ds4.wButtons = DS4_DPAD_NONE;
    }

    if (connected_) {
        update_report();
    }
}

MAA_CTRL_UNIT_NS_END
