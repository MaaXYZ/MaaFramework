#include "SeizeInput.h"

#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

void SeizeInput::ensure_foreground()
{
    if (hwnd_ == GetForegroundWindow()) {
        return;
    }
    ShowWindow(hwnd_, SW_MINIMIZE);
    ShowWindow(hwnd_, SW_RESTORE);
    SetForegroundWindow(hwnd_);
}

bool SeizeInput::click(int x, int y)
{
    LogInfo << VAR(x) << VAR(y);

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    POINT point = { x, y };
    ClientToScreen(hwnd_, &point);

    LogInfo << VAR(point.x) << VAR(point.y);

    SetCursorPos(point.x, point.y);

    INPUT inputs[2] = {};

    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));

    return true;
}

bool SeizeInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogInfo << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    if (duration <= 0) {
        LogWarn << "duration out of range" << VAR(duration);
        duration = 500;
    }

    auto start = std::chrono::steady_clock::now();
    auto now = start;

    POINT point = { x1, y1 };
    ClientToScreen(hwnd_, &point);

    INPUT input = {};

    SetCursorPos(point.x, point.y);

    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    SendInput(1, &input, sizeof(INPUT));

    constexpr double kInterval = 10; // ms
    const double steps = duration / kInterval;
    const double x_step_len = (x2 - x1) / steps;
    const double y_step_len = (y2 - y1) / steps;
    const std::chrono::milliseconds delay(static_cast<int>(kInterval));

    for (int i = 0; i < steps; ++i) {
        int tx = static_cast<int>(x1 + i * x_step_len);
        int ty = static_cast<int>(y1 + i * y_step_len);
        std::this_thread::sleep_until(now + delay);
        now = std::chrono::steady_clock::now();

        point.x = tx;
        point.y = ty;
        ClientToScreen(hwnd_, &point);
        SetCursorPos(point.x, point.y);
    }

    std::this_thread::sleep_until(now + delay);
    now = std::chrono::steady_clock::now();

    point.x = x2;
    point.y = y2;
    ClientToScreen(hwnd_, &point);
    SetCursorPos(point.x, point.y);

    std::this_thread::sleep_until(now + delay);
    now = std::chrono::steady_clock::now();

    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &input, sizeof(INPUT));

    return true;
}

bool SeizeInput::touch_down(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    std::ignore = contact;
    std::ignore = pressure;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    POINT point = { x, y };
    ClientToScreen(hwnd_, &point);
    SetCursorPos(point.x, point.y);

    INPUT input = {};

    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    SendInput(1, &input, sizeof(INPUT));

    return true;
}

bool SeizeInput::touch_move(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    std::ignore = contact;
    std::ignore = pressure;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    POINT point = { x, y };
    ClientToScreen(hwnd_, &point);

    SetCursorPos(point.x, point.y);

    return true;
}

bool SeizeInput::touch_up(int contact)
{
    LogInfo << VAR(contact);

    std::ignore = contact;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    INPUT input = {};

    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;

    SendInput(1, &input, sizeof(INPUT));

    return true;
}

bool SeizeInput::press_key(int key)
{
    LogInfo << VAR(key);

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    INPUT inputs[2] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = static_cast<WORD>(key);

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = static_cast<WORD>(key);
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));

    return true;
}

bool SeizeInput::input_text(const std::string& text)
{
    LogInfo << VAR(text);

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    ensure_foreground();

    if (std::ranges::any_of(text, [](const char& c) { //
            return static_cast<unsigned>(c) > 127;
        })) {
        LogError << "text contains non-ascii characters" << VAR(text);
        return false;
    }

    INPUT inputs[2] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    for (auto& c : text) {
        inputs[0].ki.wVk = static_cast<WORD>(c);
        inputs[1].ki.wVk = static_cast<WORD>(c);

        SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    }

    return true;
}

MAA_CTRL_UNIT_NS_END
