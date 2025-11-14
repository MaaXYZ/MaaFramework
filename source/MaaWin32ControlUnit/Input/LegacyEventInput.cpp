#include "LegacyEventInput.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/SafeWindows.hpp"

#include "InputUtils.h"

MAA_CTRL_UNIT_NS_BEGIN

void LegacyEventInput::ensure_foreground()
{
    ::MaaNS::CtrlUnitNs::ensure_foreground_and_topmost(hwnd_);
}

MaaControllerFeature LegacyEventInput::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool LegacyEventInput::click(int x, int y)
{
    LogError << "deprecated" << VAR(x) << VAR(y);
    return false;
}

bool LegacyEventInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated" << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool LegacyEventInput::touch_down(int contact, int x, int y, int pressure)
{
    POINT point = { x, y };

    if (hwnd_) {
        ensure_foreground();
        ClientToScreen(hwnd_, &point);
    }
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure) << VAR(point.x) << VAR(point.y) << VAR_VOIDP(hwnd_);

    SetCursorPos(point.x, point.y);

    // 使用旧的mouse_event API（已废弃，但某些情况下可能仍然有效）
    MouseEventFlags flags_info;
    if (!contact_to_mouse_down_flags(contact, flags_info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    mouse_event(flags_info.flags, 0, 0, flags_info.button_data, 0);

    return true;
}

bool LegacyEventInput::touch_move(int contact, int x, int y, int pressure)
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

bool LegacyEventInput::touch_up(int contact)
{
    if (hwnd_) {
        ensure_foreground();
    }
    LogInfo << VAR(contact) << VAR(hwnd_);

    MouseEventFlags flags_info;
    if (!contact_to_mouse_up_flags(contact, flags_info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    mouse_event(flags_info.flags, 0, 0, flags_info.button_data, 0);

    return true;
}

bool LegacyEventInput::click_key(int key)
{
    LogError << "deprecated" << VAR(key);
    return false;
}

bool LegacyEventInput::input_text(const std::string& text)
{
    if (hwnd_) {
        ensure_foreground();
    }

    auto u16_text = to_u16(text);
    LogInfo << VAR(text) << VAR(u16_text) << VAR(hwnd_);

    // 使用旧的keybd_event API（已废弃，但某些情况下可能仍然有效）
    for (const auto ch : u16_text) {
        // keybd_event不支持Unicode，只能发送虚拟键码
        // 这里简化处理，只发送字符码
        keybd_event(static_cast<BYTE>(ch), 0, 0, 0);
        keybd_event(static_cast<BYTE>(ch), 0, KEYEVENTF_KEYUP, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return true;
}

bool LegacyEventInput::key_down(int key)
{
    if (hwnd_) {
        ensure_foreground();
    }
    LogInfo << VAR(key) << VAR(hwnd_);

    keybd_event(static_cast<BYTE>(key), 0, 0, 0);

    return true;
}

bool LegacyEventInput::key_up(int key)
{
    if (hwnd_) {
        ensure_foreground();
    }
    LogInfo << VAR(key) << VAR(hwnd_);

    keybd_event(static_cast<BYTE>(key), 0, KEYEVENTF_KEYUP, 0);

    return true;
}

MAA_CTRL_UNIT_NS_END

