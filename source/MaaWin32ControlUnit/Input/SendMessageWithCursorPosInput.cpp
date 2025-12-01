#include "SendMessageWithCursorPosInput.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/SafeWindows.hpp"

#include "InputUtils.h"

MAA_CTRL_UNIT_NS_BEGIN

POINT SendMessageWithCursorPosInput::client_to_screen(int x, int y)
{
    POINT point = { x, y };
    if (hwnd_) {
        ClientToScreen(hwnd_, &point);
    }
    return point;
}

void SendMessageWithCursorPosInput::save_cursor_pos()
{
    GetCursorPos(&saved_cursor_pos_);
    cursor_pos_saved_ = true;
}

void SendMessageWithCursorPosInput::restore_cursor_pos()
{
    if (cursor_pos_saved_) {
        SetCursorPos(saved_cursor_pos_.x, saved_cursor_pos_.y);
        cursor_pos_saved_ = false;
    }
}

void SendMessageWithCursorPosInput::set_cursor_to_client_pos(int x, int y)
{
    POINT screen_pos = client_to_screen(x, y);
    SetCursorPos(screen_pos.x, screen_pos.y);
}

bool SendMessageWithCursorPosInput::touch_down(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    std::ignore = pressure;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    // 保存当前光标位置
    save_cursor_pos();

    // 移动光标到目标位置
    set_cursor_to_client_pos(x, y);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    MouseMessageInfo msg_info;
    if (!contact_to_mouse_down_message(contact, msg_info)) {
        LogError << "contact out of range" << VAR(contact);
        restore_cursor_pos();
        return false;
    }

    SendMessage(hwnd_, msg_info.message, msg_info.w_param, MAKELPARAM(x, y));
    last_pos_ = { x, y };

    return true;
}

bool SendMessageWithCursorPosInput::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    // 移动光标到目标位置
    set_cursor_to_client_pos(x, y);

    MouseMessageInfo msg_info;
    if (!contact_to_mouse_move_message(contact, msg_info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    SendMessage(hwnd_, msg_info.message, msg_info.w_param, MAKELPARAM(x, y));
    last_pos_ = { x, y };

    return true;
}

bool SendMessageWithCursorPosInput::touch_up(int contact)
{
    LogInfo << VAR(contact);

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

    // 恢复光标位置
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    restore_cursor_pos();

    return true;
}

bool SendMessageWithCursorPosInput::scroll(int dx, int dy)
{
    LogInfo << VAR(dx) << VAR(dy);

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    // 保存当前光标位置
    save_cursor_pos();

    // 移动光标到上次记录的位置
    set_cursor_to_client_pos(last_pos_.first, last_pos_.second);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if (dy != 0) {
        WPARAM wParam = MAKEWPARAM(0, static_cast<short>(dy));
        SendMessage(hwnd_, WM_MOUSEWHEEL, wParam, MAKELPARAM(last_pos_.first, last_pos_.second));
    }

    if (dx != 0) {
        WPARAM wParam = MAKEWPARAM(0, static_cast<short>(dx));
        SendMessage(hwnd_, WM_MOUSEHWHEEL, wParam, MAKELPARAM(last_pos_.first, last_pos_.second));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // 恢复光标位置
    restore_cursor_pos();

    return true;
}

MAA_CTRL_UNIT_NS_END
