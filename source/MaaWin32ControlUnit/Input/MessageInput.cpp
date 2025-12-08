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

bool MessageInput::send_or_post_w(UINT message, WPARAM wParam, LPARAM lParam)
{
    bool success = false;

    if (mode_ == Mode::PostMessage) {
        success = PostMessageW(hwnd_, message, wParam, lParam) != 0;
    }
    else {
        SendMessageW(hwnd_, message, wParam, lParam);
        success = true; // SendMessage 总是返回，除非窗口句柄无效
    }

    if (!success) {
        DWORD error = GetLastError();
        LogError << "Failed to" << mode_ << VAR(message) << VAR(wParam) << VAR(lParam) << VAR(error);
    }

    return success;
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
    LogError << "deprecated" << VAR(mode_) << VAR(with_cursor_pos_) << VAR(x) << VAR(y);
    return false;
}

bool MessageInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated" << VAR(mode_) << VAR(with_cursor_pos_) << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool MessageInput::touch_down(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(mode_) << VAR(with_cursor_pos_) << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    std::ignore = pressure;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    if (block_input_) {
        BlockInput(TRUE);
    }

    // 如果需要管理光标位置，保存当前位置并移动到目标位置
    if (with_cursor_pos_) {
        save_cursor_pos();
        set_cursor_to_client_pos(x, y);
    }

    // 先发送 MOVE 消息
    touch_move(contact, x, y, pressure);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    MouseMessageInfo msg_info;
    if (!contact_to_mouse_down_message(contact, msg_info)) {
        LogError << VAR(mode_) << VAR(with_cursor_pos_) << "contact out of range" << VAR(contact);
        // 错误时恢复光标位置
        if (with_cursor_pos_) {
            restore_cursor_pos();
        }
        return false;
    }

    if (!send_or_post_w(msg_info.message, msg_info.w_param, MAKELPARAM(x, y))) {
        // 错误时恢复光标位置
        if (with_cursor_pos_) {
            restore_cursor_pos();
        }
        return false;
    }

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
        LogError << VAR(mode_) << VAR(with_cursor_pos_) << "contact out of range" << VAR(contact);
        return false;
    }

    if (!send_or_post_w(msg_info.message, msg_info.w_param, MAKELPARAM(x, y))) {
        return false;
    }

    last_pos_ = { x, y };

    return true;
}

bool MessageInput::touch_up(int contact)
{
    LogInfo << VAR(mode_) << VAR(with_cursor_pos_) << VAR(contact);

    if (!hwnd_) {
        LogError << VAR(mode_) << VAR(with_cursor_pos_) << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    OnScopeLeave([this]() {
        if (block_input_) {
            BlockInput(FALSE);
        }
    });

    MouseMessageInfo msg_info;
    if (!contact_to_mouse_up_message(contact, msg_info)) {
        LogError << VAR(mode_) << VAR(with_cursor_pos_) << "contact out of range" << VAR(contact);
        return false;
    }

    if (!send_or_post_w(msg_info.message, msg_info.w_param, MAKELPARAM(last_pos_.first, last_pos_.second))) {
        return false;
    }

    // touch_up 时恢复光标位置（与 touch_down 配对）
    if (with_cursor_pos_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        restore_cursor_pos();
    }

    return true;
}

bool MessageInput::click_key(int key)
{
    LogError << "deprecated" << VAR(mode_) << VAR(with_cursor_pos_) << VAR(key);
    return false;
}

bool MessageInput::input_text(const std::string& text)
{
    LogInfo << VAR(mode_) << VAR(with_cursor_pos_) << VAR(text);

    if (!hwnd_) {
        LogError << VAR(mode_) << VAR(with_cursor_pos_) << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    // 文本输入仅发送 WM_CHAR
    for (const auto ch : to_u16(text)) {
        if (!send_or_post_w(WM_CHAR, static_cast<WPARAM>(ch), 0)) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}

bool MessageInput::key_down(int key)
{
    LogInfo << VAR(mode_) << VAR(key);

    if (!hwnd_) {
        LogError << VAR(mode_) << VAR(with_cursor_pos_) << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    LPARAM lParam = make_keydown_lparam(key);
    return send_or_post_w(WM_KEYDOWN, static_cast<WPARAM>(key), lParam);
}

bool MessageInput::key_up(int key)
{
    LogInfo << VAR(mode_) << VAR(key);

    if (!hwnd_) {
        LogError << VAR(mode_) << VAR(with_cursor_pos_) << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    LPARAM lParam = make_keyup_lparam(key);
    return send_or_post_w(WM_KEYUP, static_cast<WPARAM>(key), lParam);
}

bool MessageInput::scroll(int dx, int dy)
{
    LogInfo << VAR(mode_) << VAR(with_cursor_pos_) << VAR(dx) << VAR(dy);

    if (!hwnd_) {
        LogError << VAR(mode_) << VAR(with_cursor_pos_) << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    if (block_input_) {
        BlockInput(TRUE);
    }

    OnScopeLeave([this]() {
        if (block_input_) {
            BlockInput(FALSE);
        }
    });

    if (with_cursor_pos_) {
        // 保存当前光标位置
        save_cursor_pos();
        // 移动光标到上次记录的位置
        set_cursor_to_client_pos(last_pos_.first, last_pos_.second);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if (dy != 0) {
        WPARAM wParam = MAKEWPARAM(0, static_cast<short>(dy));
        if (!send_or_post_w(WM_MOUSEWHEEL, wParam, MAKELPARAM(last_pos_.first, last_pos_.second))) {
            if (with_cursor_pos_) {
                restore_cursor_pos();
            }
            return false;
        }
    }

    if (dx != 0) {
        WPARAM wParam = MAKEWPARAM(0, static_cast<short>(dx));
        if (!send_or_post_w(WM_MOUSEHWHEEL, wParam, MAKELPARAM(last_pos_.first, last_pos_.second))) {
            if (with_cursor_pos_) {
                restore_cursor_pos();
            }
            return false;
        }
    }

    if (with_cursor_pos_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // 恢复光标位置
        restore_cursor_pos();
    }

    return true;
}

MAA_CTRL_UNIT_NS_END
