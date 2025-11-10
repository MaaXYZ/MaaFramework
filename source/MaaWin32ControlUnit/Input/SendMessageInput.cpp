#include "SendMessageInput.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/SafeWindows.hpp"

#include "InputUtils.h"

MAA_CTRL_UNIT_NS_BEGIN

void SendMessageInput::ensure_foreground()
{
    ::MaaNS::CtrlUnitNs::ensure_foreground(hwnd_);
}

MaaControllerFeature SendMessageInput::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool SendMessageInput::click(int x, int y)
{
    LogError << "deprecated" << VAR(x) << VAR(y);
    return false;
}

bool SendMessageInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated" << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool SendMessageInput::touch_down(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    std::ignore = pressure;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    touch_move(contact, x, y, pressure);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    MouseMessageInfo msg_info;
    if (!contact_to_mouse_down_message(contact, msg_info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    SendMessage(hwnd_, msg_info.message, msg_info.w_param, MAKELPARAM(x, y));
    last_pos_ = { x, y };

    return true;
}

bool SendMessageInput::touch_move(int contact, int x, int y, int pressure)
{
    // LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    std::ignore = pressure;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    MouseMessageInfo msg_info;
    if (!contact_to_mouse_move_message(contact, msg_info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    SendMessage(hwnd_, msg_info.message, msg_info.w_param, MAKELPARAM(x, y));
    last_pos_ = { x, y };

    return true;
}

bool SendMessageInput::touch_up(int contact)
{
    LogInfo << VAR(contact);

    std::ignore = contact;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    MouseMessageInfo msg_info;
    if (!contact_to_mouse_up_message(contact, msg_info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    SendMessage(hwnd_, msg_info.message, msg_info.w_param, MAKELPARAM(last_pos_.first, last_pos_.second));

    return true;
}

bool SendMessageInput::click_key(int key)
{
    LogError << "deprecated" << VAR(key);
    return false;
}

bool SendMessageInput::input_text(const std::string& text)
{
    LogInfo << VAR(text);

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    // 文本输入仅发送 WM_CHAR
    for (const auto ch : to_u16(text)) {
        SendMessageW(hwnd_, WM_CHAR, static_cast<WPARAM>(ch), 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}

bool SendMessageInput::key_down(int key)
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    LPARAM lParam = make_keydown_lparam(key);
    SendMessageW(hwnd_, WM_KEYDOWN, static_cast<WPARAM>(key), lParam);
    return true;
}

bool SendMessageInput::key_up(int key)
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    LPARAM lParam = make_keyup_lparam(key);
    SendMessageW(hwnd_, WM_KEYUP, static_cast<WPARAM>(key), lParam);
    return true;
}

MAA_CTRL_UNIT_NS_END
