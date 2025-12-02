#include "MessageInput.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/SafeWindows.hpp"

#include "InputUtils.h"

MAA_CTRL_UNIT_NS_BEGIN

MessageInput::~MessageInput()
{
    if (block_input_) {
        BlockInput(FALSE);
    }
}

void MessageInput::ensure_foreground()
{
    ::MaaNS::CtrlUnitNs::ensure_foreground(hwnd_);
}

void MessageInput::send_or_post(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (mode_ == Mode::PostMessage) {
        PostMessage(hwnd_, message, wParam, lParam);
    }
    else {
        SendMessage(hwnd_, message, wParam, lParam);
    }
}

void MessageInput::send_or_post_w(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (mode_ == Mode::PostMessage) {
        PostMessageW(hwnd_, message, wParam, lParam);
    }
    else {
        SendMessageW(hwnd_, message, wParam, lParam);
    }
}

POINT MessageInput::client_to_screen(int x, int y)
{
    POINT point = { x, y };
    if (hwnd_) {
        ClientToScreen(hwnd_, &point);
    }
    return point;
}

void MessageInput::save_cursor_pos()
{
    GetCursorPos(&saved_cursor_pos_);
    cursor_pos_saved_ = true;
}

void MessageInput::restore_cursor_pos()
{
    if (cursor_pos_saved_) {
        SetCursorPos(saved_cursor_pos_.x, saved_cursor_pos_.y);
        cursor_pos_saved_ = false;
    }
}

void MessageInput::set_cursor_to_client_pos(int x, int y)
{
    POINT screen_pos = client_to_screen(x, y);
    SetCursorPos(screen_pos.x, screen_pos.y);
}

MaaControllerFeature MessageInput::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool MessageInput::click(int x, int y)
{
    LogError << "deprecated" << VAR(x) << VAR(y);
    return false;
}

bool MessageInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated" << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool MessageInput::touch_down(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    std::ignore = pressure;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    if (block_input_) {
        BlockInput(TRUE);
    }

    if (with_cursor_pos_) {
        // 保存当前光标位置
        save_cursor_pos();
        // 移动光标到目标位置
        set_cursor_to_client_pos(x, y);
    }

    // touch_move will use send_or_post and handle logic
    // but we call touch_move implementation logic directly or just use the code here?
    // touch_move in this class also checks flags? Yes.
    // However, existing code calls touch_move(contact, x, y, pressure) inside touch_down.
    // If I call touch_move here, it might re-set cursor pos.
    // In PostMessageInput:
    /*
    touch_move(contact, x, y, pressure);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    ...
    */
    // Let's replicate logic but be careful about double actions.
    // For MessageInput, touch_move just sends message.
    // For WithCursorPos, touch_move also sets cursor.
    // Setting cursor twice is fine.

    // But wait, in PostMessageInput::touch_down, it calls touch_move FIRST, then sleeps, then sends DOWN message.
    // touch_move sends MOVE message.
    // So order is: MOVE -> SLEEP -> DOWN.

    touch_move(contact, x, y, pressure);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    MouseMessageInfo msg_info;
    if (!contact_to_mouse_down_message(contact, msg_info)) {
        LogError << "contact out of range" << VAR(contact);
        if (with_cursor_pos_) {
            restore_cursor_pos();
        }
        if (block_input_) {
            BlockInput(FALSE);
        }
        return false;
    }

    send_or_post(msg_info.message, msg_info.w_param, MAKELPARAM(x, y));
    last_pos_ = { x, y };

    return true;
}

bool MessageInput::touch_move(int contact, int x, int y, int pressure)
{
    // LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    std::ignore = pressure;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    if (with_cursor_pos_) {
        // 移动光标到目标位置
        set_cursor_to_client_pos(x, y);
    }

    MouseMessageInfo msg_info;
    if (!contact_to_mouse_move_message(contact, msg_info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    send_or_post(msg_info.message, msg_info.w_param, MAKELPARAM(x, y));
    last_pos_ = { x, y };

    return true;
}

bool MessageInput::touch_up(int contact)
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

    send_or_post(msg_info.message, msg_info.w_param, MAKELPARAM(last_pos_.first, last_pos_.second));

    if (with_cursor_pos_) {
        // 恢复光标位置
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        restore_cursor_pos();
    }

    if (block_input_) {
        BlockInput(FALSE);
    }

    return true;
}

bool MessageInput::click_key(int key)
{
    LogError << "deprecated" << VAR(key);
    return false;
}

bool MessageInput::input_text(const std::string& text)
{
    LogInfo << VAR(text);

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    // 文本输入仅发送 WM_CHAR
    for (const auto ch : to_u16(text)) {
        send_or_post_w(WM_CHAR, static_cast<WPARAM>(ch), 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}

bool MessageInput::key_down(int key)
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    LPARAM lParam = make_keydown_lparam(key);
    send_or_post_w(WM_KEYDOWN, static_cast<WPARAM>(key), lParam);
    return true;
}

bool MessageInput::key_up(int key)
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    LPARAM lParam = make_keyup_lparam(key);
    send_or_post_w(WM_KEYUP, static_cast<WPARAM>(key), lParam);
    return true;
}

bool MessageInput::scroll(int dx, int dy)
{
    LogInfo << VAR(dx) << VAR(dy);

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    if (block_input_) {
        BlockInput(TRUE);
    }

    if (with_cursor_pos_) {
        // 保存当前光标位置
        save_cursor_pos();
        // 移动光标到上次记录的位置
        set_cursor_to_client_pos(last_pos_.first, last_pos_.second);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if (dy != 0) {
        WPARAM wParam = MAKEWPARAM(0, static_cast<short>(dy));
        send_or_post(WM_MOUSEWHEEL, wParam, MAKELPARAM(last_pos_.first, last_pos_.second));
    }

    if (dx != 0) {
        WPARAM wParam = MAKEWPARAM(0, static_cast<short>(dx));
        send_or_post(WM_MOUSEHWHEEL, wParam, MAKELPARAM(last_pos_.first, last_pos_.second));
    }

    if (with_cursor_pos_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // 恢复光标位置
        restore_cursor_pos();
    }

    if (block_input_) {
        BlockInput(FALSE);
    }

    return true;
}

MAA_CTRL_UNIT_NS_END
