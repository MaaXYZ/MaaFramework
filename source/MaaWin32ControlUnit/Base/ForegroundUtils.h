#pragma once

#include <chrono>
#include <thread>

#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

// 前台窗口是目标窗口本身，或与目标窗口属于同一个顶层 owner（如游戏自己弹出的登录框、下拉框）
inline bool is_foreground_or_owned(HWND hwnd)
{
    HWND foreground = GetForegroundWindow();
    if (!foreground) {
        return false;
    }
    if (foreground == hwnd) {
        return true;
    }

    HWND root_owner = GetAncestor(hwnd, GA_ROOTOWNER);
    return root_owner && GetAncestor(foreground, GA_ROOTOWNER) == root_owner;
}

inline void ensure_foreground_and_topmost(HWND hwnd)
{
    if (!hwnd) {
        return;
    }

    // 如果窗口不在前台，先将其置顶；owned 窗口在前台时不抢前台，否则会关闭其弹出的下拉框等控件
    if (!is_foreground_or_owned(hwnd)) {
        // 将窗口移到 Z 序顶部
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        // 尝试设置为前台窗口
        SetForegroundWindow(hwnd);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // 再次检查，如果仍然不在前台，临时置顶再取消，强制提到 Z 序最前
        // SetForegroundWindow 可能被系统前台限制拒绝，但 TOPMOST 切换不受此限制
        if (!is_foreground_or_owned(hwnd)) {
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
}

inline bool ensure_foreground_with_cooldown(HWND hwnd)
{
    constexpr DWORD kForegroundRecoveryInterval = 5000;
    static DWORD last_foreground_attempt = 0;

    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }

    // owned 窗口在前台时不抢前台，否则会让其失去焦点并关闭弹出的下拉框等控件
    if (is_foreground_or_owned(hwnd)) {
        return true;
    }

    DWORD now = GetTickCount();
    if (last_foreground_attempt != 0 && now - last_foreground_attempt < kForegroundRecoveryInterval) {
        return false;
    }

    last_foreground_attempt = now;
    ensure_foreground_and_topmost(hwnd);
    return is_foreground_or_owned(hwnd);
}

MAA_CTRL_UNIT_NS_END
