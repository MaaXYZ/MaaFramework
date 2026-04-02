#include "MessageInput.h"

#include "MaaUtils/LibraryHolder.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/SafeWindows.hpp"

#include "InputUtils.h"

#include <algorithm>
#include <mmsystem.h>

MAA_CTRL_UNIT_NS_BEGIN

namespace
{

struct NtDllHolder : public LibraryHolder<NtDllHolder>
{
};

struct User32DllHolder : public LibraryHolder<User32DllHolder>
{
};

struct ShcoreDllHolder : public LibraryHolder<ShcoreDllHolder>
{
};

void ensure_process_dpi_awareness_once()
{
    [[maybe_unused]] static const int dpi_init_once = []() {
        User32DllHolder::load_library(L"user32.dll");

        using FnCtx = BOOL WINAPI(DPI_AWARENESS_CONTEXT);
        auto fn_ctx = User32DllHolder::get_function<FnCtx>("SetProcessDpiAwarenessContext");
        if (fn_ctx && fn_ctx(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
            return 0;
        }
        if (fn_ctx && GetLastError() == ERROR_ACCESS_DENIED) {
            return 0;
        }

        ShcoreDllHolder::load_library(L"shcore.dll");

        using FnAware = HRESULT WINAPI(int);
        auto fn_aware = ShcoreDllHolder::get_function<FnAware>("SetProcessDpiAwareness");
        if (!fn_aware) {
            return 0;
        }

        fn_aware(2 /*PROCESS_PER_MONITOR_DPI_AWARE*/);
        return 0;
    }();
}

}

MessageInput::MessageInput(HWND hwnd, Config config)
    : hwnd_(hwnd)
    , config_(config)
{
    if (config_.with_window_pos) {
        tracking_thread_ = std::thread(&MessageInput::tracking_thread_func, this);
    }
}

MessageInput::~MessageInput()
{
    if (mouse_lock_follow_active_) {
        deactivate_mouse_lock_follow();
    }
    MessageInput* expected = this;
    s_active_instance_.compare_exchange_strong(expected, nullptr);
    restore_pos();
    unblock_input();
    tracking_exit_ = true;
    if (tracking_thread_.joinable()) {
        tracking_thread_.join();
    }
}

HWND MessageInput::send_activate()
{
    HWND target = get_active_hwnd();
    bool use_post = (config_.mode == Mode::PostMessage);
    ::MaaNS::CtrlUnitNs::send_activate_message(target, use_post);
    return target;
}

bool MessageInput::send_or_post_w(HWND target, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!target || !IsWindow(target)) {
        LogError << "Invalid target window" << VAR(target) << VAR(message);
        return false;
    }

    bool success = false;

    if (config_.mode == Mode::PostMessage) {
        success = PostMessageW(target, message, wParam, lParam) != 0;
    }
    else {
        SendMessageW(target, message, wParam, lParam);
        success = true;
    }

    if (!success) {
        DWORD error = GetLastError();
        LogError << "Failed to" << config_.mode << VAR(message) << VAR(wParam) << VAR(lParam) << VAR(error);
    }

    return success;
}

HWND MessageInput::get_active_hwnd()
{
    HWND root = GetAncestor(hwnd_, GA_ROOTOWNER);
    if (!root) {
        LogWarn << "GetAncestor returned nullptr, hwnd_ may be invalid" << VAR(hwnd_);
        return hwnd_;
    }
    HWND popup = GetLastActivePopup(root);
    if (popup && popup != hwnd_ && IsWindowVisible(popup)) {
        return popup;
    }
    return hwnd_;
}

LPARAM MessageInput::make_mouse_lparam(HWND target, int x, int y)
{
    if (target == hwnd_) {
        return MAKELPARAM(x, y);
    }
    POINT pt = { x, y };
    if (!ClientToScreen(hwnd_, &pt)) {
        LogError << "ClientToScreen failed in make_mouse_lparam" << VAR(hwnd_) << VAR(GetLastError());
        return MAKELPARAM(x, y);
    }
    if (!ScreenToClient(target, &pt)) {
        LogError << "ScreenToClient failed in make_mouse_lparam" << VAR(target) << VAR(GetLastError());
        return MAKELPARAM(x, y);
    }
    return MAKELPARAM(pt.x, pt.y);
}

POINT MessageInput::client_to_screen(int x, int y)
{
    POINT point = { x, y };
    if (hwnd_) {
        ClientToScreen(hwnd_, &point);
    }
    return point;
}

void MessageInput::save_cursor_pos()
{
    if (!GetCursorPos(&saved_cursor_pos_)) {
        LogError << "GetCursorPos failed" << VAR(GetLastError());
        return;
    }
    cursor_pos_saved_ = true;
}

void MessageInput::restore_cursor_pos()
{
    if (!cursor_pos_saved_) {
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if (!SetCursorPos(saved_cursor_pos_.x, saved_cursor_pos_.y)) {
        LogError << "SetCursorPos failed" << VAR(saved_cursor_pos_.x) << VAR(saved_cursor_pos_.y) << VAR(GetLastError());
    }
    cursor_pos_saved_ = false;
}

void MessageInput::save_window_pos()
{
    // 保留首次进入 WithWindowPos 会话前的位置，依赖 inactive/析构路径统一恢复并清空标记。
    if (window_pos_saved_) {
        return;
    }

    if (!hwnd_) {
        return;
    }

    if (!GetWindowRect(hwnd_, &saved_window_rect_)) {
        LogError << "GetWindowRect failed" << VAR(hwnd_) << VAR(GetLastError());
        return;
    }
    window_pos_saved_ = true;
}

void MessageInput::restore_window_pos()
{
    if (!window_pos_saved_ || !hwnd_) {
        return;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    LONG left = saved_window_rect_.left;
    LONG top = saved_window_rect_.top;

    if (!MonitorFromRect(&saved_window_rect_, MONITOR_DEFAULTTONULL)) {
        LogWarn << "saved window position is off-screen, restoring to top-left" << VAR(saved_window_rect_.left)
                << VAR(saved_window_rect_.top);
        left = 0;
        top = 0;
    }

    if (!SetWindowPos(hwnd_, nullptr, left, top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) {
        LogError << "SetWindowPos failed during restore" << VAR(hwnd_) << VAR(GetLastError());
    }
    window_pos_saved_ = false;
}

void MessageInput::start_window_tracking(int x, int y)
{
    ++tracking_generation_;
    tracking_stop_generation_ = 0;
    tracking_stop_deadline_ticks_ = 0;
    tracking_x_ = x;
    tracking_y_ = y;
    pending_mouse_x_ = 0;
    pending_mouse_y_ = 0;
    has_pending_mouse_ = false;
    s_active_instance_ = this;
    tracking_active_ = true;
}

void MessageInput::request_stop_window_tracking()
{
    if (!tracking_active_.load()) {
        return;
    }

    // 记住当前 tracking 代次，避免旧的 stop 请求在后续 touch_move 重启 tracking 后误停新一轮会话。
    tracking_stop_generation_ = tracking_generation_.load();
    tracking_stop_deadline_ticks_ = (TrackingClock::now() + std::chrono::milliseconds(10)).time_since_epoch().count();
}

void MessageInput::maybe_stop_window_tracking()
{
    if (!tracking_active_.load()) {
        return;
    }

    auto deadline_ticks = tracking_stop_deadline_ticks_.load();
    auto now = TrackingClock::now();
    if (deadline_ticks == 0 || now < TrackingClock::time_point(TrackingClock::duration(deadline_ticks))) {
        return;
    }

    // grace period 结束后先把最后一批硬件位移吃完，避免刚好落在 tracking 帧间隔中间时丢最后一小段拖动。
    if (has_pending_mouse_.load()) {
        return;
    }

    auto expected_deadline_ticks = deadline_ticks;
    // 只有清掉自己看到的 deadline 才能说明这次 stop 没有被更新的请求覆盖。
    if (!tracking_stop_deadline_ticks_.compare_exchange_strong(expected_deadline_ticks, 0)) {
        return;
    }

    auto stop_generation = tracking_stop_generation_.load();
    auto current_generation = tracking_generation_.load();
    if (stop_generation != 0 && stop_generation == current_generation) {
        stop_window_tracking();
    }
}

void MessageInput::stop_window_tracking()
{
    tracking_stop_generation_ = 0;
    tracking_stop_deadline_ticks_ = 0;
    tracking_active_ = false;
    s_active_instance_ = nullptr;
    pending_mouse_x_ = 0;
    pending_mouse_y_ = 0;
    has_pending_mouse_ = false;
}

bool MessageInput::handle_hardware_mouse_move(const MSLLHOOKSTRUCT& mouse_info)
{
    // injected 事件来自我们自己的 SetCursorPos；再参与累加会形成自我反馈，窗口会被越带越偏。
    if (mouse_info.flags & LLMHF_INJECTED) {
        return false;
    }

    if (!tracking_active_.load()) {
        return false;
    }

    // pt 是系统根据 "当前光标位置 + 硬件原始delta" 计算出的目标位置（始终物理像素）。
    // 光标被我们冻住了，所以 delta = pt - 当前冻住的光标位置。
    // 使用 GetPhysicalCursorPos 代替 GetCursorPos：前者无条件返回物理像素，
    // 不受线程 DPI 上下文影响，与 MSLLHOOKSTRUCT::pt 坐标系始终一致。
    POINT cursor;
    if (!GetPhysicalCursorPos(&cursor)) {
        LogError << "GetPhysicalCursorPos failed in mouse hook, fallback to hook point" << VAR(GetLastError());
        cursor = mouse_info.pt;
    }
    int dx = mouse_info.pt.x - cursor.x;
    int dy = mouse_info.pt.y - cursor.y;

    // 使用原子 += 累加每次的增量，不丢失任何中间移动
    pending_mouse_x_ += dx;
    pending_mouse_y_ += dy;
    has_pending_mouse_ = true;

    return true;
}

void MessageInput::save_pos()
{
    if (config_.with_cursor_pos) {
        save_cursor_pos();
    }
    else if (config_.with_window_pos) {
        save_window_pos();
    }
}

void MessageInput::finish_pos()
{
    if (config_.with_cursor_pos) {
        restore_cursor_pos();
    }
    else if (config_.with_window_pos) {
        stop_window_tracking();
    }
}

void MessageInput::restore_pos()
{
    finish_pos();

    if (config_.with_window_pos) {
        restore_window_pos();
    }
}

bool MessageInput::move_window_to_align_cursor(int x, int y)
{
    if (!hwnd_) {
        LogError << "move_window_to_align_cursor: hwnd_ is nullptr";
        return false;
    }

    POINT cursor_pos;
    if (!GetCursorPos(&cursor_pos)) {
        LogError << "GetCursorPos failed" << VAR(GetLastError());
        return false;
    }

    POINT pt = { 0, 0 };
    if (!ClientToScreen(hwnd_, &pt)) {
        LogError << "ClientToScreen failed" << VAR(hwnd_) << VAR(GetLastError());
        return false;
    }

    RECT current_rect = { 0, 0, 0, 0 };
    if (!GetWindowRect(hwnd_, &current_rect)) {
        LogError << "GetWindowRect failed" << VAR(hwnd_) << VAR(GetLastError());
        return false;
    }

    // 锚点算法：使用客户区起始位置与窗口真实坐标间的偏移量，避免由于异步调用的延迟导致的累积拉扯误差
    int border_x = pt.x - current_rect.left;
    int border_y = pt.y - current_rect.top;

    int new_left = cursor_pos.x - x - border_x;
    int new_top = cursor_pos.y - y - border_y;

    if (!SetWindowPos(hwnd_, nullptr, new_left, new_top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS)) {
        LogError << "SetWindowPos failed" << VAR(hwnd_) << VAR(new_left) << VAR(new_top) << VAR(GetLastError());
        return false;
    }

    return true;
}

LPARAM MessageInput::prepare_mouse_position(int x, int y)
{
    if (config_.with_cursor_pos) {
        // WithCursorPos 模式：移动真实光标到目标位置
        POINT screen_pos = client_to_screen(x, y);
        if (!SetCursorPos(screen_pos.x, screen_pos.y)) {
            LogError << "SetCursorPos failed" << VAR(screen_pos.x) << VAR(screen_pos.y) << VAR(GetLastError());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    else if (config_.with_window_pos) {
        start_window_tracking(x, y);
        move_window_to_align_cursor(x, y);
    }
    return MAKELPARAM(x, y);
}

// WH_MOUSE_LL 钩子回调：累加硬件鼠标位移 delta 并拦截，由追踪线程按固定帧率批量释放
LRESULT CALLBACK MessageInput::MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode != HC_ACTION || wParam != WM_MOUSEMOVE) {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    // 在关键初始化窗口期间，直接吞掉所有鼠标移动
    if (hook_block_mouse_.load()) {
        return 1;
    }

    auto* mouse_info = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
    auto* inst = s_active_instance_.load();
    if (!inst || !inst->handle_hardware_mouse_move(*mouse_info)) {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    // 拦截原始硬件鼠标移动（稍后由追踪线程通过 SetCursorPos 一次性释放累积量）
    return 1;
}

void MessageInput::open_target_process()
{
    if (target_process_handle_ || !hwnd_) {
        return;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd_, &pid);
    if (!pid) {
        LogWarn << "GetWindowThreadProcessId returned 0" << VAR(hwnd_) << VAR(GetLastError());
        return;
    }

    target_process_handle_ = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, pid);
    if (!target_process_handle_) {
        LogWarn << "OpenProcess failed" << VAR(pid) << VAR(GetLastError());
        return;
    }

    NtDllHolder::load_library(L"ntdll.dll");
}

void MessageInput::close_target_process()
{
    if (!target_process_handle_) {
        return;
    }
    CloseHandle(target_process_handle_);
    target_process_handle_ = nullptr;
}

void MessageInput::suspend_target_process()
{
    if (!target_process_handle_) {
        return;
    }

    using NtSuspendProcessFn = LONG NTAPI(HANDLE);
    static auto fn = NtDllHolder::get_function<NtSuspendProcessFn>("NtSuspendProcess");
    if (fn) {
        fn(target_process_handle_);
    }
}

void MessageInput::resume_target_process()
{
    if (!target_process_handle_) {
        return;
    }

    using NtResumeProcessFn = LONG NTAPI(HANDLE);
    static auto fn = NtDllHolder::get_function<NtResumeProcessFn>("NtResumeProcess");
    if (fn) {
        fn(target_process_handle_);
    }
}

void MessageInput::process_pending_mouse_frame()
{
    has_pending_mouse_ = false;

    // 原子读取并清零累积的 delta（exchange 保证不丢失并发写入）
    int dx = pending_mouse_x_.exchange(0);
    int dy = pending_mouse_y_.exchange(0);
    int tx = tracking_x_;
    int ty = tracking_y_;

    // 基于当前真实光标位置 + 累积 delta 计算目标光标位置
    POINT cursor;
    if (!GetCursorPos(&cursor)) {
        LogError << "GetCursorPos failed in tracking frame" << VAR(GetLastError());
        return;
    }
    int mx = cursor.x + dx;
    int my = cursor.y + dy;

    // 限制目标光标位置在虚拟主屏幕范围内，避免光标撞墙时窗口继续飞出边界导致不同步
    int vscreen_x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int vscreen_y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int vscreen_w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int vscreen_h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    mx = std::max(vscreen_x, std::min(mx, vscreen_x + vscreen_w - 1));
    my = std::max(vscreen_y, std::min(my, vscreen_y + vscreen_h - 1));

    POINT client_origin = { 0, 0 };
    if (!ClientToScreen(hwnd_, &client_origin)) {
        LogError << "ClientToScreen failed in tracking frame" << VAR(hwnd_) << VAR(GetLastError());
        return;
    }

    RECT rect;
    if (!GetWindowRect(hwnd_, &rect)) {
        LogError << "GetWindowRect failed in tracking frame" << VAR(hwnd_) << VAR(GetLastError());
        return;
    }

    int border_x = client_origin.x - rect.left;
    int border_y = client_origin.y - rect.top;
    int new_left = mx - tx - border_x;
    int new_top = my - ty - border_y;

    // 1. 挂起目标进程，避免它在窗口和光标尚未重新对齐时观测到中间态
    suspend_target_process();

    // 2. 移动窗口（SWP_ASYNCWINDOWPOS 避免阻塞 + SWP_NOSENDCHANGING 跳过同步通知）
    SetWindowPos(
        hwnd_,
        nullptr,
        new_left,
        new_top,
        0,
        0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_ASYNCWINDOWPOS);

    // 3. 释放光标到累积后的真实目标位置
    SetCursorPos(mx, my);

    // 4. 恢复目标进程（它醒来时看到的窗口和光标已完美对齐）
    resume_target_process();
}

void MessageInput::tracking_thread_func()
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    // Win32ControlUnitMgr::connect() 只覆盖创建 MessageInput 的线程；tracking 线程仍需单独设置。
    // 进程级 DPI 兜底会影响整个宿主进程，因此只初始化一次；线程级设置仍需每个 tracking 线程执行。
    ensure_process_dpi_awareness_once();
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // 将系统定时器精度提升到 1ms，确保 Sleep/MsgWait 精度
    timeBeginPeriod(1);
    bool init_reported = false;
    auto cleanup_tracking_thread = [this, &init_reported]() {
        this->cleanup_tracking_thread(init_reported);
    };
    OnScopeLeave(cleanup_tracking_thread);

    // 预先打开目标进程句柄（用于挂起/恢复）
    open_target_process();

    HHOOK hHook = SetWindowsHookExW(WH_MOUSE_LL, MouseHookProc, GetModuleHandleW(NULL), 0);
    if (!hHook) {
        LogError << "SetWindowsHookExW failed, tracking disabled" << VAR(GetLastError());
        return;
    }
    OnScopeLeave([&]() { UnhookWindowsHookEx(hHook); });

    {
        std::lock_guard lock(tracking_state_mutex_);
        tracking_thread_init_done_ = true;
        tracking_thread_init_ok_ = true;
        init_reported = true;
    }
    tracking_state_cv_.notify_all();

    // 60fps 节流：每帧间隔约 16.67ms
    using clock = std::chrono::steady_clock;
    static constexpr auto frame_interval = std::chrono::nanoseconds(1'000'000'000 / 60);
    auto last_frame = clock::now();

    MSG msg;
    while (!tracking_exit_) {
        // 消息泵：WH_MOUSE_LL 钩子回调和 WM_INPUT 消息在此线程上被系统调度
        DWORD res = MsgWaitForMultipleObjects(0, NULL, FALSE, 1, QS_ALLINPUT);
        if (res == WAIT_OBJECT_0) {
            while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    tracking_exit_ = true;
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }

        auto now = clock::now();
        bool frame_ready = (now - last_frame) >= frame_interval;

        bool ensure_rawinput = false;
        {
            std::lock_guard lock(tracking_state_mutex_);
            ensure_rawinput = rawinput_ensure_requested_;
        }
        if (ensure_rawinput) {
            bool ok = rawinput_hwnd_ || create_rawinput_window();
            {
                std::lock_guard lock(tracking_state_mutex_);
                rawinput_ensure_requested_ = false;
                rawinput_ensure_done_ = true;
                rawinput_ensure_ok_ = ok;
            }
            tracking_state_cv_.notify_all();
        }

        if (tracking_active_.load() && frame_ready) {
            if (mouse_lock_follow_active_) {
                // MouseLockFollow 模式：始终处理帧（即使没有新输入也要覆盖游戏的 SetCursorPos）
                process_mouse_lock_follow_frame();
                last_frame = now;
            }
            else if (has_pending_mouse_.load()) {
                // 普通 WithWindowPos 模式：仅在有新输入时处理
                process_pending_mouse_frame();
                last_frame = now;
            }
        }

        if (!mouse_lock_follow_active_) {
            maybe_stop_window_tracking();
        }
    }
}

void MessageInput::cleanup_tracking_thread(bool init_reported)
{
    {
        std::lock_guard lock(tracking_state_mutex_);
        if (!init_reported) {
            tracking_thread_init_done_ = true;
            tracking_thread_init_ok_ = false;
        }
        rawinput_ensure_requested_ = false;
        rawinput_ensure_done_ = true;
        rawinput_ensure_ok_ = false;
    }
    tracking_state_cv_.notify_all();
    close_target_process();
    destroy_rawinput_window();
    timeEndPeriod(1);
}

void MessageInput::check_and_block_input()
{
    if (!config_.block_input) {
        return;
    }
    BlockInput(TRUE);
}

void MessageInput::unblock_input()
{
    if (!config_.block_input) {
        return;
    }
    BlockInput(FALSE);
}

void MessageInput::inactive()
{
    LogFunc;

    if (mouse_lock_follow_active_) {
        deactivate_mouse_lock_follow();
    }

    restore_pos();
    unblock_input();
}

std::pair<int, int> MessageInput::get_target_pos() const
{
    if (last_pos_set_) {
        return last_pos_;
    }

    // 未设置时返回窗口客户区中心
    RECT rect = { };
    if (hwnd_ && GetClientRect(hwnd_, &rect)) {
        return { (rect.right - rect.left) / 2, (rect.bottom - rect.top) / 2 };
    }
    return { 0, 0 };
}

MaaControllerFeature MessageInput::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

// get_features() 返回 MaaControllerFeature_UseMouseDownAndUpInsteadOfClick，
// 上层 ControllerAgent 会使用 touch_down/touch_up 替代 click/swipe
bool MessageInput::click(int x, int y)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_up instead"
             << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << VAR(x) << VAR(y);
    return false;
}

bool MessageInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_move/touch_up instead"
             << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << VAR(x1) << VAR(y1) << VAR(x2)
             << VAR(y2) << VAR(duration);
    return false;
}

bool MessageInput::touch_down(int contact, int x, int y, int pressure)
{
    LogInfo << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << VAR(contact) << VAR(x) << VAR(y)
            << VAR(pressure);

    std::ignore = pressure;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    MouseMessageInfo move_info;
    if (!contact_to_mouse_move_message(contact, move_info)) {
        LogError << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << "contact out of range"
                 << VAR(contact);
        return false;
    }

    MouseMessageInfo down_info;
    if (!contact_to_mouse_down_message(contact, down_info)) {
        LogError << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << "contact out of range"
                 << VAR(contact);
        return false;
    }

    HWND target = send_activate();
    gesture_target_ = target;

    check_and_block_input();

    save_pos();

    prepare_mouse_position(x, y);

    LPARAM lParam = make_mouse_lparam(target, x, y);

    if (!send_or_post_w(target, move_info.message, move_info.w_param, lParam)) {
        gesture_target_ = nullptr;
        finish_pos();
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if (!send_or_post_w(target, down_info.message, down_info.w_param, lParam)) {
        gesture_target_ = nullptr;
        finish_pos();
        return false;
    }

    last_pos_ = { x, y };
    last_pos_set_ = true;

    return true;
}

bool MessageInput::touch_move(int contact, int x, int y, int pressure)
{
    // LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    std::ignore = pressure;

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    MouseMessageInfo msg_info;
    if (!contact_to_mouse_move_message(contact, msg_info)) {
        LogError << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << "contact out of range"
                 << VAR(contact);
        return false;
    }

    prepare_mouse_position(x, y);

    HWND target = (gesture_target_ && IsWindow(gesture_target_)) ? gesture_target_ : get_active_hwnd();
    LPARAM lParam = make_mouse_lparam(target, x, y);

    if (!send_or_post_w(target, msg_info.message, msg_info.w_param, lParam)) {
        gesture_target_ = nullptr;
        return false;
    }

    last_pos_ = { x, y };
    last_pos_set_ = true;

    return true;
}

bool MessageInput::touch_up(int contact)
{
    LogInfo << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << VAR(contact);

    if (!hwnd_) {
        LogError << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << "hwnd_ is nullptr";
        return false;
    }

    bool reuse_gesture = gesture_target_ && IsWindow(gesture_target_);
    HWND target = reuse_gesture ? gesture_target_ : send_activate();
    gesture_target_ = nullptr;

    OnScopeLeave([this]() { unblock_input(); });

    if (reuse_gesture) {
        bool use_post = (config_.mode == Mode::PostMessage);
        ::MaaNS::CtrlUnitNs::send_activate_message(target, use_post);
    }

    MouseMessageInfo msg_info;
    if (!contact_to_mouse_up_message(contact, msg_info)) {
        LogError << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << "contact out of range"
                 << VAR(contact);
        return false;
    }
    auto target_pos = get_target_pos();
    LPARAM lParam = make_mouse_lparam(target, target_pos.first, target_pos.second);

    if (!send_or_post_w(target, msg_info.message, msg_info.w_param, lParam)) {
        finish_pos();
        return false;
    }

    // touch_up 后继续黏住窗口一小段时间，再由 tracking 线程自行结束。
    if (config_.with_window_pos) {
        request_stop_window_tracking();
    }
    else {
        finish_pos();
    }

    return true;
}

// get_features() 返回 MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick，
// 上层 ControllerAgent 会使用 key_down/key_up 替代 click_key
bool MessageInput::click_key(int key)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick, "
                "use key_down/key_up instead"
             << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << VAR(key);
    return false;
}

bool MessageInput::input_text(const std::string& text)
{
    LogInfo << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << VAR(text);

    if (!hwnd_) {
        LogError << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << "hwnd_ is nullptr";
        return false;
    }

    HWND target = send_activate();

    bool success = true;

    for (const auto ch : to_u16(text)) {
        success &= send_or_post_w(target, WM_CHAR, static_cast<WPARAM>(ch), 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return success;
}

bool MessageInput::key_down(int key)
{
    LogInfo << VAR(config_.mode) << VAR(key);

    if (!hwnd_) {
        LogError << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << "hwnd_ is nullptr";
        return false;
    }

    HWND target = send_activate();

    LPARAM lParam = make_keydown_lparam(key);
    return send_or_post_w(target, WM_KEYDOWN, static_cast<WPARAM>(key), lParam);
}

bool MessageInput::key_up(int key)
{
    LogInfo << VAR(config_.mode) << VAR(key);

    if (!hwnd_) {
        LogError << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << "hwnd_ is nullptr";
        return false;
    }

    HWND target = send_activate();

    LPARAM lParam = make_keyup_lparam(key);
    return send_or_post_w(target, WM_KEYUP, static_cast<WPARAM>(key), lParam);
}

bool MessageInput::scroll(int dx, int dy)
{
    LogInfo << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << VAR(dx) << VAR(dy);

    if (!hwnd_) {
        LogError << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << "hwnd_ is nullptr";
        return false;
    }

    HWND target = send_activate();

    check_and_block_input();
    OnScopeLeave([this]() { unblock_input(); });

    auto target_pos = get_target_pos();

    save_pos();

    prepare_mouse_position(target_pos.first, target_pos.second);
    POINT screen_pos = client_to_screen(target_pos.first, target_pos.second);
    LPARAM lParam = MAKELPARAM(screen_pos.x, screen_pos.y);
    bool success = true;

    if (dy != 0) {
        WPARAM wParam = MAKEWPARAM(0, static_cast<short>(dy));
        success &= send_or_post_w(target, WM_MOUSEWHEEL, wParam, lParam);
    }

    if (dx != 0) {
        WPARAM wParam = MAKEWPARAM(0, static_cast<short>(dx));
        success &= send_or_post_w(target, WM_MOUSEHWHEEL, wParam, lParam);
    }

    if (config_.with_window_pos) {
        request_stop_window_tracking();
    }
    else {
        finish_pos();
    }

    return success;
}

bool MessageInput::relative_move(int dx, int dy)
{
    if (!mouse_lock_follow_active_) {
        LogError << "relative_move is only supported when mouse_lock_follow is active";
        return false;
    }

    if (dx == 0 && dy == 0) {
        return true;
    }

    // 标记：这次 SendInput 产生的 WM_INPUT 不要被 RawInput handler 对冲
    counter_pending_++;

    INPUT input = { };
    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;

    if (SendInput(1, &input, sizeof(INPUT)) != 1) {
        counter_pending_.fetch_sub(1);
        LogError << "SendInput failed for relative_move" << VAR(dx) << VAR(dy) << VAR(GetLastError());
        return false;
    }

    return true;
}

bool MessageInput::set_mouse_lock_follow(bool enabled)
{
    LogInfo << VAR(enabled) << VAR(mouse_lock_follow_active_.load());

    if (enabled && !mouse_lock_follow_active_) {
        return activate_mouse_lock_follow();
    }

    if (!enabled && mouse_lock_follow_active_) {
        deactivate_mouse_lock_follow();
    }

    return true;
}

bool MessageInput::ensure_tracking_thread()
{
    auto wait_for_init = [this]() {
        std::unique_lock lock(tracking_state_mutex_);
        tracking_state_cv_.wait(lock, [this]() { return tracking_thread_init_done_ || tracking_exit_.load(); });
        return tracking_thread_init_done_ && tracking_thread_init_ok_;
    };

    if (tracking_thread_.joinable()) {
        if (wait_for_init()) {
            return true;
        }
        tracking_thread_.join();
    }

    {
        std::lock_guard lock(tracking_state_mutex_);
        tracking_thread_init_done_ = false;
        tracking_thread_init_ok_ = false;
        rawinput_ensure_requested_ = false;
        rawinput_ensure_done_ = false;
        rawinput_ensure_ok_ = false;
    }
    tracking_exit_ = false;
    tracking_thread_ = std::thread(&MessageInput::tracking_thread_func, this);
    if (!config_.with_window_pos) {
        tracking_thread_started_for_lock_follow_ = true;
    }
    return wait_for_init();
}

bool MessageInput::ensure_rawinput_window()
{
    {
        std::lock_guard lock(tracking_state_mutex_);
        if (!tracking_thread_init_done_ || !tracking_thread_init_ok_) {
            return false;
        }
        rawinput_ensure_requested_ = true;
        rawinput_ensure_done_ = false;
        rawinput_ensure_ok_ = false;
    }

    std::unique_lock lock(tracking_state_mutex_);
    tracking_state_cv_.wait(lock, [this]() { return rawinput_ensure_done_ || !tracking_thread_init_ok_ || tracking_exit_.load(); });
    return rawinput_ensure_done_ && rawinput_ensure_ok_;
}

bool MessageInput::compute_window_center_on_cursor(const POINT& cursor, int& out_left, int& out_top)
{
    RECT client_rect;
    if (!GetClientRect(hwnd_, &client_rect)) {
        LogError << "GetClientRect failed" << VAR(GetLastError());
        return false;
    }

    int client_w = client_rect.right - client_rect.left;
    int client_h = client_rect.bottom - client_rect.top;

    POINT client_origin = { 0, 0 };
    if (!ClientToScreen(hwnd_, &client_origin)) {
        LogError << "ClientToScreen failed" << VAR(hwnd_) << VAR(GetLastError());
        return false;
    }

    RECT win_rect;
    if (!GetWindowRect(hwnd_, &win_rect)) {
        LogError << "GetWindowRect failed" << VAR(hwnd_) << VAR(GetLastError());
        return false;
    }

    int border_x = client_origin.x - win_rect.left;
    int border_y = client_origin.y - win_rect.top;
    out_left = cursor.x - client_w / 2 - border_x;
    out_top = cursor.y - client_h / 2 - border_y;
    return true;
}

bool MessageInput::activate_mouse_lock_follow()
{
    LogInfo << "Activating mouse lock follow mode";

    if (!hwnd_) {
        LogError << "Cannot activate mouse lock follow: hwnd_ is nullptr";
        return false;
    }

    // 此函数在 action runner 线程执行，必须临时切换到 Per-Monitor DPI Aware V2，
    // 否则 GetCursorPos/GetWindowRect 返回虚拟像素，与追踪线程的物理像素不匹配。
    auto prev_dpi_ctx = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    OnScopeLeave([&]() {
        if (prev_dpi_ctx) {
            SetThreadDpiAwarenessContext(prev_dpi_ctx);
        }
    });

    if (!ensure_tracking_thread()) {
        LogError << "Failed to start mouse tracking thread for mouse lock follow";
        return false;
    }

    if (!ensure_rawinput_window()) {
        LogError << "Failed to create RawInput window for mouse lock follow";
        return false;
    }

    save_window_pos();
    if (!window_pos_saved_) {
        LogError << "Failed to save window position before activating mouse lock follow" << VAR(hwnd_);
        return false;
    }

    POINT cursor;
    if (!GetPhysicalCursorPos(&cursor)) {
        LogError << "GetPhysicalCursorPos failed" << VAR(GetLastError());
        return false;
    }

    int new_left = 0, new_top = 0;
    if (!compute_window_center_on_cursor(cursor, new_left, new_top)) {
        return false;
    }

    bool activated = false;
    bool window_moved = false;
    tracking_stop_generation_ = 0;
    tracking_stop_deadline_ticks_ = 0;
    hook_block_mouse_ = true;
    OnScopeLeave([&]() {
        hook_block_mouse_ = false;
        if (activated) {
            return;
        }

        stop_window_tracking();
        mouse_lock_follow_active_ = false;
        counter_pending_ = 0;
        if (window_moved) {
            restore_window_pos();
        }
    });

    if (!SetWindowPos(hwnd_, nullptr, new_left, new_top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) {
        LogError << "SetWindowPos failed during mouse lock follow activation" << VAR(hwnd_) << VAR(new_left) << VAR(new_top)
                 << VAR(GetLastError());
        return false;
    }
    window_moved = true;
    bool settled = false;
    for (int i = 0; i < 10; ++i) {
        RECT cur;
        if (GetWindowRect(hwnd_, &cur) && std::abs(cur.left - new_left) <= 1 && std::abs(cur.top - new_top) <= 1) {
            settled = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!settled) {
        LogWarn << "SetWindowPos did not settle within 100ms" << VAR(hwnd_) << VAR(new_left) << VAR(new_top);
    }

    lock_anchor_cursor_ = cursor;
    if (!GetWindowRect(hwnd_, &lock_anchor_window_)) {
        LogError << "GetWindowRect failed after mouse lock follow activation move" << VAR(hwnd_) << VAR(GetLastError());
        return false;
    }
    lock_offset_x_ = 0;
    lock_offset_y_ = 0;

    send_activate();

    pending_mouse_x_ = 0;
    pending_mouse_y_ = 0;
    has_pending_mouse_ = false;
    counter_pending_ = 0;

    mouse_lock_follow_active_ = true;
    s_active_instance_ = this;
    tracking_active_ = true;
    activated = true;

    LogInfo << "Mouse lock follow activated" << VAR(lock_anchor_cursor_.x) << VAR(lock_anchor_cursor_.y) << VAR(lock_anchor_window_.left)
            << VAR(lock_anchor_window_.top);
    return true;
}

void MessageInput::deactivate_mouse_lock_follow()
{
    LogInfo << "Deactivating mouse lock follow mode";

    // 与 activate 同理，restore_window_pos 调用 SetWindowPos 需要物理像素坐标。
    auto prev_dpi_ctx = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    OnScopeLeave([&]() {
        if (prev_dpi_ctx) {
            SetThreadDpiAwarenessContext(prev_dpi_ctx);
        }
    });

    mouse_lock_follow_active_ = false;
    MessageInput* expected = this;
    s_active_instance_.compare_exchange_strong(expected, nullptr);
    stop_window_tracking();
    counter_pending_ = 0;

    restore_window_pos();

    // 如果追踪线程是为 lock follow 单独启动的，停止它
    if (tracking_thread_started_for_lock_follow_ && !config_.with_window_pos) {
        tracking_exit_ = true;
        if (tracking_thread_.joinable()) {
            tracking_thread_.join();
        }
        tracking_thread_started_for_lock_follow_ = false;
    }
}

void MessageInput::process_mouse_lock_follow_frame()
{
    has_pending_mouse_ = false;

    int dx = pending_mouse_x_.exchange(0);
    int dy = pending_mouse_y_.exchange(0);

    lock_offset_x_ += dx;
    lock_offset_y_ += dy;

    int mx = lock_anchor_cursor_.x + lock_offset_x_;
    int my = lock_anchor_cursor_.y + lock_offset_y_;

    int vsx = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int vsy = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int vsw = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int vsh = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    mx = std::clamp(mx, vsx, vsx + vsw - 1);
    my = std::clamp(my, vsy, vsy + vsh - 1);
    lock_offset_x_ = mx - lock_anchor_cursor_.x;
    lock_offset_y_ = my - lock_anchor_cursor_.y;

    int new_left = lock_anchor_window_.left + lock_offset_x_;
    int new_top = lock_anchor_window_.top + lock_offset_y_;

    suspend_target_process();

    SetWindowPos(
        hwnd_,
        nullptr,
        new_left,
        new_top,
        0,
        0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_ASYNCWINDOWPOS);

    SetCursorPos(mx, my);

    resume_target_process();
}

bool MessageInput::create_rawinput_window()
{
    WNDCLASSEXW wc = { };
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = RawInputWndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"MaaMouseLockFollowRawInput";

    // 注册可能失败（如果已经注册过），忽略错误
    RegisterClassExW(&wc);

    rawinput_hwnd_ = CreateWindowExW(0, wc.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, wc.hInstance, NULL);
    if (!rawinput_hwnd_) {
        LogError << "CreateWindowExW for RawInput failed" << VAR(GetLastError());
        return false;
    }

    // 注册接收鼠标 RawInput（RIDEV_INPUTSINK 确保后台也能收到）
    RAWINPUTDEVICE rid = { };
    rid.usUsagePage = 0x01; // HID_USAGE_PAGE_GENERIC
    rid.usUsage = 0x02;     // HID_USAGE_GENERIC_MOUSE
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = rawinput_hwnd_;
    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        LogError << "RegisterRawInputDevices failed" << VAR(GetLastError());
        DestroyWindow(rawinput_hwnd_);
        rawinput_hwnd_ = nullptr;
        return false;
    }

    LogInfo << "RawInput window created for mouse lock follow";
    return true;
}

void MessageInput::destroy_rawinput_window()
{
    if (!rawinput_hwnd_) {
        return;
    }

    RAWINPUTDEVICE rid = { };
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = RIDEV_REMOVE;
    rid.hwndTarget = NULL;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));

    DestroyWindow(rawinput_hwnd_);
    rawinput_hwnd_ = nullptr;
}

void MessageInput::send_counter_move(int raw_dx, int raw_dy)
{
    if (raw_dx == 0 && raw_dy == 0) {
        return;
    }

    counter_pending_++;

    INPUT counter = { };
    counter.type = INPUT_MOUSE;
    counter.mi.dx = -raw_dx;
    counter.mi.dy = -raw_dy;
    counter.mi.dwFlags = MOUSEEVENTF_MOVE;
    if (SendInput(1, &counter, sizeof(INPUT)) != 1) {
        counter_pending_.fetch_sub(1);
        LogError << "SendInput failed for counter move" << VAR(raw_dx) << VAR(raw_dy) << VAR(GetLastError());
    }
}

bool MessageInput::handle_rawinput_message(LPARAM lParam)
{
    if (!mouse_lock_follow_active_) {
        return false;
    }

    UINT size = 0;
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0 || size == 0
        || size > sizeof(RAWINPUT)) {
        return false;
    }

    RAWINPUT raw = { };
    UINT copied = size;
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &copied, sizeof(RAWINPUTHEADER)) != size) {
        return false;
    }

    if (raw.header.dwType != RIM_TYPEMOUSE) {
        return false;
    }

    const auto& mouse = raw.data.mouse;
    if ((mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0) {
        return false;
    }
    if (mouse.lLastX == 0 && mouse.lLastY == 0) {
        return false;
    }

    if (consume_synthetic_rawinput()) {
        return true;
    }

    send_counter_move(mouse.lLastX, mouse.lLastY);
    return true;
}

bool MessageInput::consume_synthetic_rawinput()
{
    int prev = counter_pending_.load();
    while (prev > 0) {
        if (counter_pending_.compare_exchange_weak(prev, prev - 1)) {
            return true;
        }
    }
    return false;
}

LRESULT CALLBACK MessageInput::RawInputWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg != WM_INPUT) {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    auto* inst = s_active_instance_.load();
    if (!inst) {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    inst->handle_rawinput_message(lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

MAA_CTRL_UNIT_NS_END
