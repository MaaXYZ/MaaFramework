#include "SendMessageInput.h"

#include "Utils/Codec.h"
#include "Utils/Logger.h"
#include "Utils/MicroControl.hpp"
#include "Utils/Platform.h"
#include "Utils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

bool SendMessageInput::click(int x, int y)
{
    LogInfo << VAR(x) << VAR(y);

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    SendMessage(hwnd_, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
    SendMessage(hwnd_, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(x, y));

    return true;
}

bool SendMessageInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogInfo << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    if (duration <= 0) {
        LogWarn << "duration out of range" << VAR(duration);
        duration = 200;
    }

    micro_swipe(
        x1,
        y2,
        x2,
        y2,
        duration,
        [&](int x, int y) { SendMessage(hwnd_, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y)); },
        [&](int x, int y) { SendMessage(hwnd_, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(x, y)); },
        [&](int x, int y) { SendMessage(hwnd_, WM_LBUTTONUP, MK_LBUTTON, MAKELPARAM(x, y)); });

    return true;
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
        message = WM_XBUTTONUP;
        w_param = MK_XBUTTON1;
        break;
    case 4:
        message = WM_XBUTTONUP;
        w_param = MK_XBUTTON2;
        break;
    default:
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    SendMessage(hwnd_, message, w_param, MAKELPARAM(x, y));

    return true;
}

bool SendMessageInput::touch_move(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

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
    WPARAM w_param = MK_LBUTTON;

    switch (contact) {
    case 0:
        message = WM_LBUTTONUP;
        w_param = MK_LBUTTON;
        break;
    case 1:
        message = WM_RBUTTONUP;
        w_param = MK_RBUTTON;
        break;
    case 2:
        message = WM_MBUTTONUP;
        w_param = MK_MBUTTON;
        break;
    case 3:
        message = WM_XBUTTONUP;
        w_param = MK_XBUTTON1;
        break;
    case 4:
        message = WM_XBUTTONUP;
        w_param = MK_XBUTTON2;
        break;
    default:
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    SendMessage(hwnd_, WM_LBUTTONUP, w_param, 0);

    return true;
}

bool SendMessageInput::press_key(int key)
{
    LogInfo << VAR(key);

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    SendMessageW(hwnd_, WM_KEYDOWN, key, 0);
    SendMessageW(hwnd_, WM_KEYUP, key, 0);

    return true;
}

bool SendMessageInput::input_text(const std::string& text)
{
    LogInfo << VAR(text);

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    for (const auto ch : to_u16(text)) {
        SendMessageW(hwnd_, WM_KEYDOWN, ch, 0);
        SendMessageW(hwnd_, WM_CHAR, ch, 0);
        SendMessageW(hwnd_, WM_KEYUP, ch, 0);
    }
    return true;
}

MAA_CTRL_UNIT_NS_END
