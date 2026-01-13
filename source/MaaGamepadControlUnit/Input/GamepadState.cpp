#include "GamepadState.h"

#include <algorithm>

MAA_CTRL_UNIT_NS_BEGIN

using namespace vigem;

// XboxGamepadState

void XboxGamepadState::press_button(int button)
{
    report_.wButtons |= static_cast<uint16_t>(button & 0xFFFF);
}

void XboxGamepadState::release_button(int button)
{
    report_.wButtons &= ~static_cast<uint16_t>(button & 0xFFFF);
}

void XboxGamepadState::set_left_stick(int x, int y)
{
    report_.sThumbLX = static_cast<int16_t>(std::clamp(x, -32768, 32767));
    report_.sThumbLY = static_cast<int16_t>(std::clamp(y, -32768, 32767));
}

void XboxGamepadState::set_right_stick(int x, int y)
{
    report_.sThumbRX = static_cast<int16_t>(std::clamp(x, -32768, 32767));
    report_.sThumbRY = static_cast<int16_t>(std::clamp(y, -32768, 32767));
}

void XboxGamepadState::set_left_trigger(uint8_t value)
{
    report_.bLeftTrigger = value;
}

void XboxGamepadState::set_right_trigger(uint8_t value)
{
    report_.bRightTrigger = value;
}

void XboxGamepadState::reset()
{
    report_ = XusbReport {};
}

uint32_t XboxGamepadState::update(
    void* client,
    void* target,
    const boost::function<FnVigemTargetX360Update>& x360_update,
    const boost::function<FnVigemTargetDs4Update>& /*ds4_update*/)
{
    return x360_update(client, target, report_);
}

// Ds4GamepadState

void Ds4GamepadState::press_button(int button)
{
    report_.wButtons |= convert_button(button);
    report_.bSpecial |= convert_special(button);
    update_dpad(button, true);
}

void Ds4GamepadState::release_button(int button)
{
    report_.wButtons &= ~convert_button(button);
    report_.bSpecial &= ~convert_special(button);
    update_dpad(button, false);
}

void Ds4GamepadState::set_left_stick(int x, int y)
{
    report_.bThumbLX = static_cast<uint8_t>(std::clamp((x + 32768) * 255 / 65535, 0, 255));
    report_.bThumbLY = static_cast<uint8_t>(std::clamp((y + 32768) * 255 / 65535, 0, 255));
}

void Ds4GamepadState::set_right_stick(int x, int y)
{
    report_.bThumbRX = static_cast<uint8_t>(std::clamp((x + 32768) * 255 / 65535, 0, 255));
    report_.bThumbRY = static_cast<uint8_t>(std::clamp((y + 32768) * 255 / 65535, 0, 255));
}

void Ds4GamepadState::set_left_trigger(uint8_t value)
{
    report_.bTriggerL = value;
}

void Ds4GamepadState::set_right_trigger(uint8_t value)
{
    report_.bTriggerR = value;
}

void Ds4GamepadState::reset()
{
    report_ = Ds4Report {};
    dpad_state_ = 0;
}

uint32_t Ds4GamepadState::update(
    void* client,
    void* target,
    const boost::function<FnVigemTargetX360Update>& /*x360_update*/,
    const boost::function<FnVigemTargetDs4Update>& ds4_update)
{
    return ds4_update(client, target, report_);
}

uint16_t Ds4GamepadState::convert_button(int button)
{
    uint16_t result = 0;

    if (button & XBOX_A) {
        result |= DS4_CROSS;
    }
    if (button & XBOX_B) {
        result |= DS4_CIRCLE;
    }
    if (button & XBOX_X) {
        result |= DS4_SQUARE;
    }
    if (button & XBOX_Y) {
        result |= DS4_TRIANGLE;
    }
    if (button & XBOX_LEFT_SHOULDER) {
        result |= DS4_L1;
    }
    if (button & XBOX_RIGHT_SHOULDER) {
        result |= DS4_R1;
    }
    if (button & XBOX_LEFT_THUMB) {
        result |= DS4_L_THUMB;
    }
    if (button & XBOX_RIGHT_THUMB) {
        result |= DS4_R_THUMB;
    }
    if (button & XBOX_START) {
        result |= DS4_OPTIONS;
    }
    if (button & XBOX_BACK) {
        result |= DS4_SHARE;
    }

    return result;
}

uint8_t Ds4GamepadState::convert_special(int button)
{
    uint8_t result = 0;

    if (button & XBOX_GUIDE) {
        result |= DS4_SPECIAL_PS;
    }
    if (button & MaaGamepadButton_PS) {
        result |= DS4_SPECIAL_PS;
    }
    if (button & MaaGamepadButton_TOUCHPAD) {
        result |= DS4_SPECIAL_TOUCHPAD;
    }

    return result;
}

void Ds4GamepadState::update_dpad(int button, bool pressed)
{
    // Xbox D-pad uses bit flags, DS4 D-pad uses a 4-bit value (0-8)
    // We track D-pad state separately and compute the DS4 value
    uint8_t dpad_mask = 0;
    if (button & XBOX_DPAD_UP) {
        dpad_mask |= 0x01;
    }
    if (button & XBOX_DPAD_RIGHT) {
        dpad_mask |= 0x02;
    }
    if (button & XBOX_DPAD_DOWN) {
        dpad_mask |= 0x04;
    }
    if (button & XBOX_DPAD_LEFT) {
        dpad_mask |= 0x08;
    }

    if (dpad_mask == 0) {
        return;
    }

    if (pressed) {
        dpad_state_ |= dpad_mask;
    }
    else {
        dpad_state_ &= ~dpad_mask;
    }

    // Clear D-pad bits (lower 4 bits) and set new value
    report_.wButtons = (report_.wButtons & 0xFFF0) | compute_dpad_value();
}

uint8_t Ds4GamepadState::compute_dpad_value() const
{
    // DS4 D-pad encoding: N=0, NE=1, E=2, SE=3, S=4, SW=5, W=6, NW=7, None=8
    bool up = dpad_state_ & 0x01;
    bool right = dpad_state_ & 0x02;
    bool down = dpad_state_ & 0x04;
    bool left = dpad_state_ & 0x08;

    // Handle conflicting directions (both pressed = neither active)
    if (up && down) {
        up = down = false;
    }
    if (left && right) {
        left = right = false;
    }

    if (up && right) {
        return 1;
    }
    if (right && down) {
        return 3;
    }
    if (down && left) {
        return 5;
    }
    if (left && up) {
        return 7;
    }
    if (up) {
        return 0;
    }
    if (right) {
        return 2;
    }
    if (down) {
        return 4;
    }
    if (left) {
        return 6;
    }
    return 8;
}

// Factory

std::unique_ptr<GamepadState> create_gamepad_state(MaaGamepadType type)
{
    if (type == MaaGamepadType_Xbox360) {
        return std::make_unique<XboxGamepadState>();
    }
    return std::make_unique<Ds4GamepadState>();
}

MAA_CTRL_UNIT_NS_END
