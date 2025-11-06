#include "PostMessageInput.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

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
        w_param = MK_XBUTTON1;
        break;
    case 4:
        message = WM_XBUTTONDOWN;
        w_param = MK_XBUTTON2;
        break;
    default:
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    // PostMessage是异步的，不等待消息处理完成
    PostMessage(hwnd_, message, w_param, MAKELPARAM(x, y));

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

    PostMessage(hwnd_, message, w_param, MAKELPARAM(x, y));

    return true;
}

bool PostMessageInput::touch_up(int contact)
{
    LogInfo << VAR(contact);

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
        w_param = MAKEWPARAM(MK_XBUTTON1, XBUTTON1);
        break;
    case 4:
        message = WM_XBUTTONUP;
        w_param = MAKEWPARAM(MK_XBUTTON2, XBUTTON2);
        break;
    default:
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    PostMessage(hwnd_, message, w_param, 0);

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

    for (const auto ch : to_u16(text)) {
        PostMessageW(hwnd_, WM_KEYDOWN, ch, 0);
        PostMessageW(hwnd_, WM_CHAR, ch, 0);
        PostMessageW(hwnd_, WM_KEYUP, ch, 0);
    }
    return true;
}

bool PostMessageInput::key_down(int key)
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    PostMessageW(hwnd_, WM_KEYDOWN, key, 0);
    return true;
}

bool PostMessageInput::key_up(int key)
{
    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    PostMessageW(hwnd_, WM_KEYUP, key, 0);
    return true;
}

MAA_CTRL_UNIT_NS_END

