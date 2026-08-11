#include "ForegroundUtils.h"

#include <atomic>
#include <chrono>
#include <thread>

MAA_CTRL_UNIT_NS_BEGIN

namespace
{
class ScopedThreadInputAttachment
{
public:
    ScopedThreadInputAttachment(DWORD source_thread_id, DWORD target_thread_id)
        : source_thread_id_(source_thread_id)
        , target_thread_id_(target_thread_id)
    {
        if (!source_thread_id_ || !target_thread_id_ || source_thread_id_ == target_thread_id_) {
            return;
        }

        attached_ = AttachThreadInput(source_thread_id_, target_thread_id_, TRUE) != FALSE;
    }

    ~ScopedThreadInputAttachment()
    {
        if (attached_) {
            AttachThreadInput(source_thread_id_, target_thread_id_, FALSE);
        }
    }

    ScopedThreadInputAttachment(const ScopedThreadInputAttachment&) = delete;
    ScopedThreadInputAttachment& operator=(const ScopedThreadInputAttachment&) = delete;

private:
    DWORD source_thread_id_ = 0;
    DWORD target_thread_id_ = 0;
    bool attached_ = false;
};

bool wait_for_foreground(HWND hwnd, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (GetForegroundWindow() != hwnd && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return GetForegroundWindow() == hwnd;
}

void restore_window(HWND hwnd)
{
    if (!IsIconic(hwnd) && IsWindowVisible(hwnd)) {
        return;
    }

    ShowWindowAsync(hwnd, IsIconic(hwnd) ? SW_RESTORE : SW_SHOW);

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
    while ((IsIconic(hwnd) || !IsWindowVisible(hwnd)) && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void request_foreground(HWND hwnd, bool force_z_order)
{
    constexpr UINT kWindowPosFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW;

    if (force_z_order) {
        const bool was_topmost = (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, kWindowPosFlags);
        if (!was_topmost) {
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, kWindowPosFlags);
        }
    }
    else {
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, kWindowPosFlags);
    }

    BringWindowToTop(hwnd);
    SetForegroundWindow(hwnd);
}
}

bool ensure_foreground_and_topmost(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }

    if (hwnd == GetForegroundWindow()) {
        return true;
    }

    restore_window(hwnd);
    request_foreground(hwnd, false);
    if (wait_for_foreground(hwnd, std::chrono::milliseconds(20))) {
        return true;
    }

    const HWND foreground_window = GetForegroundWindow();
    const DWORD current_thread_id = GetCurrentThreadId();
    const DWORD foreground_thread_id = foreground_window ? GetWindowThreadProcessId(foreground_window, nullptr) : 0;

    // SetForegroundWindow is restricted for background processes. Sharing the foreground input queue
    // gives this bounded retry the same activation context without leaving the queues attached.
    MSG message = { };
    PeekMessageW(&message, nullptr, 0, 0, PM_NOREMOVE);
    {
        ScopedThreadInputAttachment foreground_attachment(current_thread_id, foreground_thread_id);
        request_foreground(hwnd, true);
    }

    return wait_for_foreground(hwnd, std::chrono::milliseconds(50));
}

bool ensure_foreground_with_cooldown(HWND hwnd)
{
    constexpr DWORD kForegroundRecoveryInterval = 5000;
    static std::atomic<DWORD> last_foreground_attempt = 0;

    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }

    if (hwnd == GetForegroundWindow()) {
        return true;
    }

    const DWORD now = GetTickCount();
    DWORD previous_attempt = last_foreground_attempt.load(std::memory_order_relaxed);
    while (true) {
        if (previous_attempt != 0 && now - previous_attempt < kForegroundRecoveryInterval) {
            return false;
        }
        if (last_foreground_attempt.compare_exchange_weak(previous_attempt, now, std::memory_order_relaxed)) {
            break;
        }
    }

    return ensure_foreground_and_topmost(hwnd);
}

MAA_CTRL_UNIT_NS_END
