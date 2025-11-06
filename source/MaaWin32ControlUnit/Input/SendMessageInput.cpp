#include "SendMessageInput.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

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

    UINT message = WM_LBUTTONDOWN;
    WPARAM w_param = MK_LBUTTON;

    switch (contact) {
    case 0:
        message = WM_LBUTTONDOWN;
        w_param = MK_LBUTTON;
        break;
    case 1:
        message = WM_RBUTTONDOWN;
        w_param = MK_RBUTTON;
        break;
    case 2:
        message = WM_MBUTTONDOWN;
        w_param = MK_MBUTTON;
        break;
    case 3:
        message = WM_XBUTTONDOWN;
        w_param = MAKEWPARAM(MK_XBUTTON1, XBUTTON1);
        break;
    case 4:
        message = WM_XBUTTONDOWN;
        w_param = MAKEWPARAM(MK_XBUTTON2, XBUTTON2);
        break;
    default:
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    SendMessage(hwnd_, message, w_param, MAKELPARAM(x, y));
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

    UINT message = WM_MOUSEMOVE;
    WPARAM w_param = MK_LBUTTON;

    switch (contact) {
    case 0:
        message = WM_MOUSEMOVE;
        w_param = MK_LBUTTON;
        break;
    case 1:
        message = WM_MOUSEMOVE;
        w_param = MK_RBUTTON;
        break;
    case 2:
        message = WM_MOUSEMOVE;
        w_param = MK_MBUTTON;
        break;
    case 3:
        message = WM_MOUSEMOVE;
        w_param = MK_XBUTTON1;
        break;
    case 4:
        message = WM_MOUSEMOVE;
        w_param = MK_XBUTTON2;
        break;
    default:
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    SendMessage(hwnd_, message, w_param, MAKELPARAM(x, y));
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

    UINT message = WM_LBUTTONUP;
    WPARAM w_param = 0;

    switch (contact) {
    case 0:
        message = WM_LBUTTONUP;
        w_param = 0;
        break;
    case 1:
        message = WM_RBUTTONUP;
        w_param = 0;
        break;
    case 2:
        message = WM_MBUTTONUP;
        w_param = 0;
        break;
    case 3:
        message = WM_XBUTTONUP;
        w_param = MAKEWPARAM(0, XBUTTON1);
        break;
    case 4:
        message = WM_XBUTTONUP;
        w_param = MAKEWPARAM(0, XBUTTON2);
        break;
    default:
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    SendMessage(hwnd_, message, w_param, MAKELPARAM(last_pos_.first, last_pos_.second));

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

    UINT sc = MapVirtualKeyW(static_cast<UINT>(key), MAPVK_VK_TO_VSC);
    LPARAM lParam = 1 | (static_cast<LPARAM>(sc) << 16);
    SendMessageW(hwnd_, WM_KEYDOWN, static_cast<WPARAM>(key), lParam);
    return true;
}

bool SendMessageInput::key_up(int key)
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    UINT sc = MapVirtualKeyW(static_cast<UINT>(key), MAPVK_VK_TO_VSC);
    LPARAM lParam = (1 | (static_cast<LPARAM>(sc) << 16) | (1 << 30) | (1 << 31));
    SendMessageW(hwnd_, WM_KEYUP, static_cast<WPARAM>(key), lParam);
    return true;
}

MAA_CTRL_UNIT_NS_END
