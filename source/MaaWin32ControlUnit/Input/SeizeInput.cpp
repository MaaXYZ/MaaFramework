#include "SeizeInput.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/SafeWindows.hpp"

#include "InputUtils.h"

MAA_CTRL_UNIT_NS_BEGIN

void SeizeInput::ensure_foreground()
{
    ::MaaNS::CtrlUnitNs::ensure_foreground_and_topmost(hwnd_);
}

MaaControllerFeature SeizeInput::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool SeizeInput::click(int x, int y)
{
    LogError << "deprecated" << VAR(x) << VAR(y);
    return false;
}

bool SeizeInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated" << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool SeizeInput::touch_down(int contact, int x, int y, int pressure)
{
    POINT point = { x, y };

    if (hwnd_) {
        ensure_foreground();
        ClientToScreen(hwnd_, &point);
    }
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure) << VAR(point.x) << VAR(point.y) << VAR_VOIDP(hwnd_);

    SetCursorPos(point.x, point.y);

    INPUT input = {};
    input.type = INPUT_MOUSE;

    MouseEventFlags flags_info;
    if (!contact_to_mouse_down_flags(contact, flags_info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    input.mi.dwFlags = flags_info.flags;
    input.mi.mouseData = flags_info.button_data;

    SendInput(1, &input, sizeof(INPUT));

    return true;
}

bool SeizeInput::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = contact;
    std::ignore = pressure;

    POINT point = { x, y };

    if (hwnd_) {
        ensure_foreground();
        ClientToScreen(hwnd_, &point);
    }
    // LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure) << VAR(point.x) << VAR(point.y) << VAR_VOIDP(hwnd_);

    SetCursorPos(point.x, point.y);

    return true;
}

bool SeizeInput::touch_up(int contact)
{
    if (hwnd_) {
        ensure_foreground();
    }
    LogInfo << VAR(contact) << VAR(hwnd_);

    INPUT input = {};
    input.type = INPUT_MOUSE;

    MouseEventFlags flags_info;
    if (!contact_to_mouse_up_flags(contact, flags_info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    input.mi.dwFlags = flags_info.flags;
    input.mi.mouseData = flags_info.button_data;

    SendInput(1, &input, sizeof(INPUT));

    return true;
}

bool SeizeInput::click_key(int key)
{
    LogError << "deprecated" << VAR(key);
    return false;
}

bool SeizeInput::input_text(const std::string& text)
{
    if (hwnd_) {
        ensure_foreground();
    }

    auto u16_text = to_u16(text);
    LogInfo << VAR(text) << VAR(u16_text) << VAR(hwnd_);

    std::vector<INPUT> input_vec;

    for (const auto ch : u16_text) {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.dwFlags = KEYEVENTF_UNICODE;
        input.ki.wScan = ch;

        input_vec.emplace_back(input);

        input.ki.dwFlags |= KEYEVENTF_KEYUP;

        input_vec.emplace_back(input);
    }

    UINT written = SendInput(static_cast<UINT>(input_vec.size()), input_vec.data(), sizeof(INPUT));

    if (written != input_vec.size()) {
        LogError << VAR(written) << VAR(input_vec.size()) << VAR(u16_text.size());
        return false;
    }
    return true;
}

bool SeizeInput::key_down(int key)
{
    if (hwnd_) {
        ensure_foreground();
    }
    LogInfo << VAR(key) << VAR(hwnd_);

    INPUT inputs[1] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = static_cast<WORD>(key);

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));

    return true;
}

bool SeizeInput::key_up(int key)
{
    if (hwnd_) {
        ensure_foreground();
    }
    LogInfo << VAR(key) << VAR(hwnd_);

    INPUT inputs[1] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = static_cast<WORD>(key);
    inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));

    return true;
}

MAA_CTRL_UNIT_NS_END
