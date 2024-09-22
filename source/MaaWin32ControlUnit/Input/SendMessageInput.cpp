#include "SendMessageInput.h"

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
        duration = 500;
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

    std::ignore = contact;
    std::ignore = pressure;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    SendMessage(hwnd_, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));

    return true;
}

bool SendMessageInput::touch_move(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    std::ignore = contact;
    std::ignore = pressure;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    SendMessage(hwnd_, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(x, y));

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

    SendMessage(hwnd_, WM_LBUTTONUP, MK_LBUTTON, 0);

    return true;
}

bool SendMessageInput::press_key(int key)
{
    LogInfo << VAR(key);

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    SendMessage(hwnd_, WM_KEYDOWN, key, 0);
    SendMessage(hwnd_, WM_KEYUP, key, 0);

    return true;
}

bool SendMessageInput::input_text(const std::string& text)
{
    LogInfo << VAR(text);

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    auto osstr = to_osstring(text);
    SendMessage(hwnd_, WM_SETTEXT, NULL, (LPARAM)(osstr.c_str()));

    return true;
}

MAA_CTRL_UNIT_NS_END
