#include "PostMessageInput.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/SafeWindows.hpp"

#include "InputUtils.h"

MAA_CTRL_UNIT_NS_BEGIN

void PostMessageInput::ensure_foreground()
{
    ::MaaNS::CtrlUnitNs::ensure_foreground(hwnd_);
}

MaaControllerFeature PostMessageInput::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool PostMessageInput::click(int x, int y)
{
    LogError << "deprecated" << VAR(x) << VAR(y);
    return false;
}

bool PostMessageInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated" << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool PostMessageInput::touch_down(int contact, int x, int y, int pressure)
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

    // PostMessage是异步的，不等待消息处理完成
    PostMessage(hwnd_, msg_info.message, msg_info.w_param, MAKELPARAM(x, y));
    last_pos_ = { x, y };

    return true;
}

bool PostMessageInput::touch_move(int contact, int x, int y, int pressure)
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

    PostMessage(hwnd_, msg_info.message, msg_info.w_param, MAKELPARAM(x, y));
    last_pos_ = { x, y };

    return true;
}

bool PostMessageInput::touch_up(int contact)
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

    PostMessage(hwnd_, msg_info.message, msg_info.w_param, MAKELPARAM(last_pos_.first, last_pos_.second));

    return true;
}

bool PostMessageInput::click_key(int key)
{
    LogError << "deprecated" << VAR(key);
    return false;
}

bool PostMessageInput::input_text(const std::string& text)
{
    LogInfo << VAR(text);

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    // 文本输入仅发送 WM_CHAR，更符合 Win32 语义（WM_KEYDOWN/UP 由系统生成并经 TranslateMessage 转为 WM_CHAR）
    for (const auto ch : to_u16(text)) {
        PostMessageW(hwnd_, WM_CHAR, static_cast<WPARAM>(ch), 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}

bool PostMessageInput::key_down(int key)
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    // 构造更接近系统生成的 lParam：重复计数=1，扫描码填充
    LPARAM lParam = make_keydown_lparam(key);
    PostMessageW(hwnd_, WM_KEYDOWN, static_cast<WPARAM>(key), lParam);
    return true;
}

bool PostMessageInput::key_up(int key)
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    LPARAM lParam = make_keyup_lparam(key);
    PostMessageW(hwnd_, WM_KEYUP, static_cast<WPARAM>(key), lParam);
    return true;
}

MAA_CTRL_UNIT_NS_END
