#include "PostThreadMessageInput.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/SafeWindows.hpp"

#include "InputUtils.h"

MAA_CTRL_UNIT_NS_BEGIN

PostThreadMessageInput::PostThreadMessageInput(HWND hwnd)
    : hwnd_(hwnd)
{
    if (hwnd_) {
        thread_id_ = GetWindowThreadProcessId(hwnd_, nullptr);
    }
}

void PostThreadMessageInput::ensure_foreground()
{
    ::MaaNS::CtrlUnitNs::ensure_foreground(hwnd_);
}

std::pair<int, int> PostThreadMessageInput::get_target_pos() const
{
    if (last_pos_set_) {
        return last_pos_;
    }

    // 未设置时返回窗口客户区中心
    RECT rect = {};
    if (hwnd_ && GetClientRect(hwnd_, &rect)) {
        return { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
    }
    return { 0, 0 };
}

MaaControllerFeature PostThreadMessageInput::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool PostThreadMessageInput::click(int x, int y)
{
    LogError << "deprecated" << VAR(x) << VAR(y);
    return false;
}

bool PostThreadMessageInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated" << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool PostThreadMessageInput::touch_down(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    std::ignore = pressure;

    if (!thread_id_) {
        LogError << "thread_id_ is 0";
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

    // 发送到目标线程队列
    PostThreadMessage(thread_id_, msg_info.message, msg_info.w_param, MAKELPARAM(x, y));
    last_pos_ = { x, y };
    last_pos_set_ = true;

    return true;
}

bool PostThreadMessageInput::touch_move(int contact, int x, int y, int pressure)
{
    // LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    std::ignore = pressure;

    if (!thread_id_) {
        LogError << "thread_id_ is 0";
        return false;
    }

    MouseMessageInfo msg_info;
    if (!contact_to_mouse_move_message(contact, msg_info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    PostThreadMessage(thread_id_, msg_info.message, msg_info.w_param, MAKELPARAM(x, y));
    last_pos_ = { x, y };
    last_pos_set_ = true;

    return true;
}

bool PostThreadMessageInput::touch_up(int contact)
{
    LogInfo << VAR(contact);

    if (!thread_id_) {
        LogError << "thread_id_ is 0";
        return false;
    }

    ensure_foreground();

    MouseMessageInfo msg_info;
    if (!contact_to_mouse_up_message(contact, msg_info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    auto target_pos = get_target_pos();
    PostThreadMessage(thread_id_, msg_info.message, msg_info.w_param, MAKELPARAM(target_pos.first, target_pos.second));

    return true;
}

bool PostThreadMessageInput::click_key(int key)
{
    LogError << "deprecated" << VAR(key);
    return false;
}

bool PostThreadMessageInput::input_text(const std::string& text)
{
    LogInfo << VAR(text);

    if (!thread_id_) {
        LogError << "thread_id_ is 0";
        return false;
    }

    ensure_foreground();

    // 文本输入仅发送 WM_CHAR
    for (const auto ch : to_u16(text)) {
        PostThreadMessage(thread_id_, WM_CHAR, static_cast<WPARAM>(ch), 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return true;
}

bool PostThreadMessageInput::key_down(int key)
{
    if (!thread_id_) {
        LogError << "thread_id_ is 0";
        return false;
    }

    ensure_foreground();

    LPARAM lParam = make_keydown_lparam(key);
    PostThreadMessage(thread_id_, WM_KEYDOWN, static_cast<WPARAM>(key), lParam);
    return true;
}

bool PostThreadMessageInput::key_up(int key)
{
    if (!thread_id_) {
        LogError << "thread_id_ is 0";
        return false;
    }

    ensure_foreground();

    LPARAM lParam = make_keyup_lparam(key);
    PostThreadMessage(thread_id_, WM_KEYUP, static_cast<WPARAM>(key), lParam);
    return true;
}

bool PostThreadMessageInput::scroll(int dx, int dy)
{
    LogInfo << VAR(dx) << VAR(dy);

    if (!thread_id_) {
        LogError << "thread_id_ is 0";
        return false;
    }

    ensure_foreground();

    auto target_pos = get_target_pos();

    if (dy != 0) {
        WPARAM wParam = MAKEWPARAM(0, static_cast<short>(dy));
        PostThreadMessage(thread_id_, WM_MOUSEWHEEL, wParam, MAKELPARAM(target_pos.first, target_pos.second));
    }

    if (dx != 0) {
        WPARAM wParam = MAKEWPARAM(0, static_cast<short>(dx));
        PostThreadMessage(thread_id_, WM_MOUSEHWHEEL, wParam, MAKELPARAM(target_pos.first, target_pos.second));
    }

    return true;
}

MAA_CTRL_UNIT_NS_END
