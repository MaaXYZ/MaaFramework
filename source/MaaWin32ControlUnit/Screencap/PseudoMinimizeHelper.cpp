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
    SetLastError(0);
    original_ex_style_ = GetWindowLongPtr(hwnd_, GWL_EXSTYLE);
    if (original_ex_style_ == 0 && GetLastError() != 0) {
        LogError << "GetWindowLongPtr failed" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
    }
    had_layered_style_ = (original_ex_style_ & WS_EX_LAYERED) != 0;

    // 如果窗口原本就有 WS_EX_LAYERED，尝试读取原始透明度
    if (had_layered_style_) {
        DWORD flags = 0;
        COLORREF color_key = 0;
        if (!GetLayeredWindowAttributes(hwnd_, &color_key, &original_alpha_, &flags)) {
            LogWarn << "GetLayeredWindowAttributes failed, defaulting alpha to 255" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
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
        if (!ShowWindow(hwnd_, SW_MINIMIZE)) {
            LogWarn << "ShowWindow(SW_MINIMIZE) failed on stop" << VAR_VOIDP(hwnd_);
        }
    }

    LogInfo << "PseudoMinimizeHelper stopped" << VAR_VOIDP(hwnd_);
}

void PseudoMinimizeHelper::ensure_not_minimized()
{
    if (!IsWindow(hwnd_)) {
        return;
    }

    if (IsIconic(hwnd_)) {
        apply_pseudo_minimize();
    }
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

        // ensure_not_minimized() 已在 screencap 前同步处理最小化→伪最小化，
        // 此处仅处理伪最小化→恢复（用户重新激活窗口时）
        if (pseudo_minimized_ && GetForegroundWindow() == hwnd_) {
            revert_pseudo_minimize();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void PseudoMinimizeHelper::apply_pseudo_minimize()
{
    LogInfo << "Applying pseudo-minimize" << VAR_VOIDP(hwnd_);

    // 1. 添加 WS_EX_LAYERED 和 WS_EX_TRANSPARENT 样式
    SetLastError(0);
    LONG_PTR ex_style = GetWindowLongPtr(hwnd_, GWL_EXSTYLE);
    if (ex_style == 0 && GetLastError() != 0) {
        LogError << "GetWindowLongPtr failed in apply" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
        return;
    }
    SetLastError(0);
    if (SetWindowLongPtr(hwnd_, GWL_EXSTYLE, ex_style | WS_EX_LAYERED | WS_EX_TRANSPARENT) == 0 && GetLastError() != 0) {
        LogError << "SetWindowLongPtr failed in apply" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
        return;
    }

    // 2. 设置完全透明
    if (!SetLayeredWindowAttributes(hwnd_, 0, 0, LWA_ALPHA)) {
        LogError << "SetLayeredWindowAttributes failed in apply" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
        return;
    }

    // 3. 以不激活方式还原窗口，使截图方式能工作
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);

    pseudo_minimized_ = true;
}

void PseudoMinimizeHelper::revert_pseudo_minimize()
{
    LogInfo << "Reverting pseudo-minimize" << VAR_VOIDP(hwnd_);

    // 1. 直接恢复到保存的原始扩展样式，避免位操作导致的样式丢失
    SetLastError(0);
    if (SetWindowLongPtr(hwnd_, GWL_EXSTYLE, original_ex_style_) == 0 && GetLastError() != 0) {
        LogError << "SetWindowLongPtr failed in revert" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
    }

    // 2. 恢复透明度（需在恢复样式之后，因为原样式可能不含 WS_EX_LAYERED）
    if (had_layered_style_) {
        if (!SetLayeredWindowAttributes(hwnd_, 0, original_alpha_, LWA_ALPHA)) {
            LogError << "SetLayeredWindowAttributes failed in revert" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
        }
    }

    pseudo_minimized_ = false;
}

MAA_CTRL_UNIT_NS_END
