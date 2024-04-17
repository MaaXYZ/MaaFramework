#include "SendMessageInput.h"

#include "Utils/Logger.h"
#include "Utils/SafeWindows.hpp"
#include "Utils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

bool SendMessageInput::click(int x, int y)
{
    std::ignore = x;
    std::ignore = y;

    return false;
}

bool SendMessageInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    std::ignore = x1;
    std::ignore = y1;
    std::ignore = x2;
    std::ignore = y2;
    std::ignore = duration;

    return false;
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