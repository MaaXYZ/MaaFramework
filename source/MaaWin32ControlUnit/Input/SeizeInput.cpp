#include "SeizeInput.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/SafeWindows.hpp"

#include "InputUtils.h"

MAA_CTRL_UNIT_NS_BEGIN

SeizeInput::~SeizeInput()
{
    if (block_input_) {
        BlockInput(FALSE);
    }
}

void SeizeInput::ensure_foreground()
{
    ::MaaNS::CtrlUnitNs::ensure_foreground_and_topmost(hwnd_);
}

std::pair<int, int> SeizeInput::get_target_pos() const
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

MaaControllerFeature SeizeInput::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool SeizeInput::click(int x, int y)
{
    LogError << "deprecated" << VAR(x) << VAR(y);
    return false;
}

bool SeizeInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated" << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool SeizeInput::touch_down(int contact, int x, int y, int pressure)
{
    POINT point = { x, y };

    if (hwnd_) {
        ensure_foreground();
        ClientToScreen(hwnd_, &point);
    }
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure) << VAR(point.x) << VAR(point.y) << VAR_VOIDP(hwnd_);

    if (block_input_) {
        BlockInput(TRUE);
    }

    SetCursorPos(point.x, point.y);

    INPUT input = {};
    input.type = INPUT_MOUSE;

    MouseEventFlags flags_info;
    if (!contact_to_mouse_down_flags(contact, flags_info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    input.mi.dwFlags = flags_info.flags;
    input.mi.mouseData = flags_info.button_data;

    SendInput(1, &input, sizeof(INPUT));

    last_pos_ = { x, y };
    last_pos_set_ = true;

    return true;
}

bool SeizeInput::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = contact;
    std::ignore = pressure;

    POINT point = { x, y };

    if (hwnd_) {
        ensure_foreground();
        ClientToScreen(hwnd_, &point);
    }
    // LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure) << VAR(point.x) << VAR(point.y) << VAR_VOIDP(hwnd_);

    // 使用 SendInput + MOUSEEVENTF_MOVE + MOUSEEVENTF_ABSOLUTE 移动光标
    // 需要将屏幕坐标转换为 0-65535 范围的归一化坐标
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);

    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = static_cast<LONG>((point.x * 65535) / screen_width);
    input.mi.dy = static_cast<LONG>((point.y * 65535) / screen_height);
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;

    SendInput(1, &input, sizeof(INPUT));

    last_pos_ = { x, y };
    last_pos_set_ = true;

    return true;
}

bool SeizeInput::touch_up(int contact)
{
    if (hwnd_) {
        ensure_foreground();
    }
    LogInfo << VAR(contact) << VAR(hwnd_);

    OnScopeLeave([this]() {
        if (block_input_) {
            BlockInput(FALSE);
        }
    });

    INPUT input = {};
    input.type = INPUT_MOUSE;

    MouseEventFlags flags_info;
    if (!contact_to_mouse_up_flags(contact, flags_info)) {
        LogError << "contact out of range" << VAR(contact);
        return false;
    }

    input.mi.dwFlags = flags_info.flags;
    input.mi.mouseData = flags_info.button_data;

    SendInput(1, &input, sizeof(INPUT));

    return true;
}

bool SeizeInput::click_key(int key)
{
    LogError << "deprecated" << VAR(key);
    return false;
}

bool SeizeInput::input_text(const std::string& text)
{
    if (hwnd_) {
        ensure_foreground();
    }

    auto u16_text = to_u16(text);
    LogInfo << VAR(text) << VAR(u16_text) << VAR(hwnd_);

    if (block_input_) {
        BlockInput(TRUE);
    }

    OnScopeLeave([this]() {
        if (block_input_) {
            BlockInput(FALSE);
        }
    });

    std::vector<INPUT> input_vec;

    for (const auto ch : u16_text) {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.dwFlags = KEYEVENTF_UNICODE;
        input.ki.wScan = ch;

        input_vec.emplace_back(input);

        input.ki.dwFlags |= KEYEVENTF_KEYUP;

        input_vec.emplace_back(input);
    }

    UINT written = SendInput(static_cast<UINT>(input_vec.size()), input_vec.data(), sizeof(INPUT));

    if (written != input_vec.size()) {
        LogError << VAR(written) << VAR(input_vec.size()) << VAR(u16_text.size());
        return false;
    }
    return true;
}

bool SeizeInput::key_down(int key)
{
    if (hwnd_) {
        ensure_foreground();
    }
    LogInfo << VAR(key) << VAR(hwnd_);

    if (block_input_) {
        BlockInput(TRUE);
    }

    INPUT inputs[1] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = static_cast<WORD>(key);

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));

    return true;
}

bool SeizeInput::key_up(int key)
{
    if (hwnd_) {
        ensure_foreground();
    }
    LogInfo << VAR(key) << VAR(hwnd_);

    OnScopeLeave([this]() {
        if (block_input_) {
            BlockInput(FALSE);
        }
    });

    INPUT inputs[1] = {};

    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = static_cast<WORD>(key);
    inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));

    return true;
}

bool SeizeInput::scroll(int dx, int dy)
{
    LogInfo << VAR(dx) << VAR(dy);

    if (hwnd_) {
        ensure_foreground();
    }

    if (block_input_) {
        BlockInput(TRUE);
    }

    OnScopeLeave([this]() {
        if (block_input_) {
            BlockInput(FALSE);
        }
    });

    // 移动光标到目标位置
    auto [target_x, target_y] = get_target_pos();
    POINT point = { target_x, target_y };
    if (hwnd_) {
        ClientToScreen(hwnd_, &point);
    }
    SetCursorPos(point.x, point.y);

    INPUT input = {};
    input.type = INPUT_MOUSE;

    if (dy != 0) {
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = static_cast<DWORD>(dy);
        SendInput(1, &input, sizeof(INPUT));
    }

    if (dx != 0) {
        input.mi.dwFlags = MOUSEEVENTF_HWHEEL;
        input.mi.mouseData = static_cast<DWORD>(dx);
        SendInput(1, &input, sizeof(INPUT));
    }

    return true;
}

MAA_CTRL_UNIT_NS_END
