#pragma once

#include <chrono>
#include <thread>

#include "Common/Conf.h"
#include "ControlUnit/ControlUnitAPI.h"
#include "MaaUtils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

// 发送 WM_ACTIVATE 消息激活窗口（用于后台消息发送方式）
// 让目标窗口认为自己被激活，但不实际改变前台窗口
inline void send_activate_message(HWND hwnd, bool use_post = false)
{
    if (!hwnd) {
        return;
    }
    // WM_ACTIVATE + WA_ACTIVE，lParam 为 0 表示没有前一个窗口
    if (use_post) {
        PostMessageW(hwnd, WM_ACTIVATE, WA_ACTIVE, 0);
    }
    else {
        SendMessageW(hwnd, WM_ACTIVATE, WA_ACTIVE, 0);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

// 窗口激活并置顶工具函数（强化版本，用于需要前台的物理输入方式）
// 用于 LegacyEventInput 和 SeizeInput，因为它们使用 SendInput/mouse_event 等物理输入 API
inline void ensure_foreground_and_topmost(HWND hwnd)
{
    if (!hwnd) {
        return;
    }

    // 如果窗口不在前台，先将其置顶
    if (hwnd != GetForegroundWindow()) {
        // 将窗口移到 Z 序顶部
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        // 尝试设置为前台窗口
        SetForegroundWindow(hwnd);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // 再次检查，如果仍然不在前台，再次置顶
        if (hwnd != GetForegroundWindow()) {
            SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

// Contact 到 WM_* 消息的转换结果
struct MouseMessageInfo
{
    UINT message = 0;
    WPARAM w_param = 0;
};

// 将 contact ID 转换为鼠标按下消息
inline bool contact_to_mouse_down_message(int contact, MouseMessageInfo& info)
{
    switch (contact) {
    case 0:
        info.message = WM_LBUTTONDOWN;
        info.w_param = MK_LBUTTON;
        return true;
    case 1:
        info.message = WM_RBUTTONDOWN;
        info.w_param = MK_RBUTTON;
        return true;
    case 2:
        info.message = WM_MBUTTONDOWN;
        info.w_param = MK_MBUTTON;
        return true;
    case 3:
        info.message = WM_XBUTTONDOWN;
        info.w_param = MAKEWPARAM(MK_XBUTTON1, XBUTTON1);
        return true;
    case 4:
        info.message = WM_XBUTTONDOWN;
        info.w_param = MAKEWPARAM(MK_XBUTTON2, XBUTTON2);
        return true;
    default:
        return false;
    }
}

// 将 contact ID 转换为鼠标移动消息
inline bool contact_to_mouse_move_message(int contact, MouseMessageInfo& info)
{
    switch (contact) {
    case 0:
        info.message = WM_MOUSEMOVE;
        info.w_param = MK_LBUTTON;
        return true;
    case 1:
        info.message = WM_MOUSEMOVE;
        info.w_param = MK_RBUTTON;
        return true;
    case 2:
        info.message = WM_MOUSEMOVE;
        info.w_param = MK_MBUTTON;
        return true;
    case 3:
        info.message = WM_MOUSEMOVE;
        info.w_param = MK_XBUTTON1;
        return true;
    case 4:
        info.message = WM_MOUSEMOVE;
        info.w_param = MK_XBUTTON2;
        return true;
    default:
        return false;
    }
}

// 将 contact ID 转换为鼠标抬起消息
inline bool contact_to_mouse_up_message(int contact, MouseMessageInfo& info)
{
    switch (contact) {
    case 0:
        info.message = WM_LBUTTONUP;
        info.w_param = 0;
        return true;
    case 1:
        info.message = WM_RBUTTONUP;
        info.w_param = 0;
        return true;
    case 2:
        info.message = WM_MBUTTONUP;
        info.w_param = 0;
        return true;
    case 3:
        info.message = WM_XBUTTONUP;
        info.w_param = MAKEWPARAM(0, XBUTTON1);
        return true;
    case 4:
        info.message = WM_XBUTTONUP;
        info.w_param = MAKEWPARAM(0, XBUTTON2);
        return true;
    default:
        return false;
    }
}

// MOUSEEVENTF 标志和按钮数据
struct MouseEventFlags
{
    DWORD flags;
    DWORD button_data;
};

// 将 contact ID 转换为 MOUSEEVENTF 按下标志（用于 SendInput/mouse_event）
inline bool contact_to_mouse_down_flags(int contact, MouseEventFlags& flags_info)
{
    switch (contact) {
    case 0:
        flags_info.flags = MOUSEEVENTF_LEFTDOWN;
        flags_info.button_data = 0;
        return true;
    case 1:
        flags_info.flags = MOUSEEVENTF_RIGHTDOWN;
        flags_info.button_data = 0;
        return true;
    case 2:
        flags_info.flags = MOUSEEVENTF_MIDDLEDOWN;
        flags_info.button_data = 0;
        return true;
    case 3:
        flags_info.flags = MOUSEEVENTF_XDOWN;
        flags_info.button_data = XBUTTON1;
        return true;
    case 4:
        flags_info.flags = MOUSEEVENTF_XDOWN;
        flags_info.button_data = XBUTTON2;
        return true;
    default:
        return false;
    }
}

// 将 contact ID 转换为 MOUSEEVENTF 抬起标志（用于 SendInput/mouse_event）
inline bool contact_to_mouse_up_flags(int contact, MouseEventFlags& flags_info)
{
    switch (contact) {
    case 0:
        flags_info.flags = MOUSEEVENTF_LEFTUP;
        flags_info.button_data = 0;
        return true;
    case 1:
        flags_info.flags = MOUSEEVENTF_RIGHTUP;
        flags_info.button_data = 0;
        return true;
    case 2:
        flags_info.flags = MOUSEEVENTF_MIDDLEUP;
        flags_info.button_data = 0;
        return true;
    case 3:
        flags_info.flags = MOUSEEVENTF_XUP;
        flags_info.button_data = XBUTTON1;
        return true;
    case 4:
        flags_info.flags = MOUSEEVENTF_XUP;
        flags_info.button_data = XBUTTON2;
        return true;
    default:
        return false;
    }
}

// 构造 WM_KEYDOWN 的 lParam
inline LPARAM make_keydown_lparam(int key)
{
    UINT sc = MapVirtualKeyW(static_cast<UINT>(key), MAPVK_VK_TO_VSC);
    return 1 | (static_cast<LPARAM>(sc) << 16);
}

// 构造 WM_KEYUP 的 lParam
inline LPARAM make_keyup_lparam(int key)
{
    UINT sc = MapVirtualKeyW(static_cast<UINT>(key), MAPVK_VK_TO_VSC);
    // 置位先前状态与转换状态位
    return (1 | (static_cast<LPARAM>(sc) << 16) | (1 << 30) | (1 << 31));
}

MAA_CTRL_UNIT_NS_END
