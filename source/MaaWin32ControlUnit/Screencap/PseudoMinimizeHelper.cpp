#include "PseudoMinimizeHelper.h"

#include <chrono>

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

PseudoMinimizeHelper::PseudoMinimizeHelper(HWND hwnd)
    : hwnd_(hwnd)
{
}

PseudoMinimizeHelper::~PseudoMinimizeHelper()
{
    stop();
}

void PseudoMinimizeHelper::start()
{
    if (running_) {
        return;
    }

    // 保存窗口原始扩展样式
    original_ex_style_ = GetWindowLongPtr(hwnd_, GWL_EXSTYLE);
    had_layered_style_ = (original_ex_style_ & WS_EX_LAYERED) != 0;

    // 如果窗口原本就有 WS_EX_LAYERED，尝试读取原始透明度
    if (had_layered_style_) {
        DWORD flags = 0;
        COLORREF color_key = 0;
        if (GetLayeredWindowAttributes(hwnd_, &color_key, &original_alpha_, &flags)) {
            // 成功读取
        }
        else {
            original_alpha_ = 255;
        }
    }
    else {
        original_alpha_ = 255;
    }

    running_ = true;
    monitor_thread_ = std::thread(&PseudoMinimizeHelper::monitor_thread_func, this);

    LogInfo << "PseudoMinimizeHelper started" << VAR_VOIDP(hwnd_);
}

void PseudoMinimizeHelper::stop()
{
    if (!running_) {
        return;
    }

    running_ = false;

    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }

    // 确保退出时恢复窗口状态
    if (pseudo_minimized_) {
        revert_pseudo_minimize();
        // 用户期望窗口处于最小化状态，所以恢复属性后再最小化
        ShowWindow(hwnd_, SW_MINIMIZE);
    }

    LogInfo << "PseudoMinimizeHelper stopped" << VAR_VOIDP(hwnd_);
}

void PseudoMinimizeHelper::monitor_thread_func()
{
    LogFunc;

    while (running_) {
        if (!IsWindow(hwnd_)) {
            LogWarn << "Window no longer valid, stopping monitor";
            pseudo_minimized_ = false;
            break;
        }

        if (!pseudo_minimized_ && IsIconic(hwnd_)) {
            // 窗口被最小化了，执行伪最小化
            apply_pseudo_minimize();
        }
        else if (pseudo_minimized_ && GetForegroundWindow() == hwnd_) {
            // 窗口重新变为前台（用户点击了任务栏图标），恢复正常
            revert_pseudo_minimize();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void PseudoMinimizeHelper::apply_pseudo_minimize()
{
    LogInfo << "Applying pseudo-minimize" << VAR_VOIDP(hwnd_);

    // 1. 添加 WS_EX_LAYERED 和 WS_EX_TRANSPARENT 样式
    LONG_PTR ex_style = GetWindowLongPtr(hwnd_, GWL_EXSTYLE);
    SetWindowLongPtr(hwnd_, GWL_EXSTYLE, ex_style | WS_EX_LAYERED | WS_EX_TRANSPARENT);

    // 2. 设置完全透明
    SetLayeredWindowAttributes(hwnd_, 0, 0, LWA_ALPHA);

    // 3. 以不激活方式还原窗口，使截图方式能工作
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);

    pseudo_minimized_ = true;
}

void PseudoMinimizeHelper::revert_pseudo_minimize()
{
    LogInfo << "Reverting pseudo-minimize" << VAR_VOIDP(hwnd_);

    // 1. 直接恢复到保存的原始扩展样式，避免位操作导致的样式丢失
    SetWindowLongPtr(hwnd_, GWL_EXSTYLE, original_ex_style_);

    // 2. 恢复透明度（需在恢复样式之后，因为原样式可能不含 WS_EX_LAYERED）
    if (had_layered_style_) {
        SetLayeredWindowAttributes(hwnd_, 0, original_alpha_, LWA_ALPHA);
    }

    pseudo_minimized_ = false;
}

MAA_CTRL_UNIT_NS_END
