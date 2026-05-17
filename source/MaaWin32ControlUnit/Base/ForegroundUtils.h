#pragma once

#include <chrono>
#include <thread>

#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

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

MAA_CTRL_UNIT_NS_END
