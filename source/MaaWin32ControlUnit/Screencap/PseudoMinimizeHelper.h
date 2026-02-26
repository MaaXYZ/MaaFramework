#pragma once

#include <atomic>
#include <thread>

#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

// 伪最小化辅助类：当目标窗口被最小化时，将其设为透明+点击穿透并还原，
// 使截图方式能继续工作。当窗口重新变为前台时恢复正常属性。
// 析构时确保窗口属性被完全恢复，不会让用户"丢失"窗口。
class PseudoMinimizeHelper
{
public:
    explicit PseudoMinimizeHelper(HWND hwnd);
    ~PseudoMinimizeHelper();

    PseudoMinimizeHelper(const PseudoMinimizeHelper&) = delete;
    PseudoMinimizeHelper& operator=(const PseudoMinimizeHelper&) = delete;

    void start();
    void stop();

    // 截图前同步调用：若窗口处于最小化状态，立即执行伪最小化并等待窗口就绪
    void ensure_not_minimized();

    bool is_pseudo_minimized() const { return pseudo_minimized_; }

private:
    void monitor_thread_func();
    void apply_pseudo_minimize();
    void revert_pseudo_minimize();

    HWND hwnd_ = nullptr;

    std::atomic<bool> running_ { false };
    std::atomic<bool> pseudo_minimized_ { false };
    std::thread monitor_thread_;

    // 保存窗口原始属性，用于恢复
    LONG_PTR original_ex_style_ = 0;
    BYTE original_alpha_ = 255;
    bool had_layered_style_ = false;
};

MAA_CTRL_UNIT_NS_END
