#include "MessageInput.h"

#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/SafeWindows.hpp"

#include "InputUtils.h"

#include <mmsystem.h>

// 如果未定义该常量，则预先定义，用于确保 Windows 10 下的多显示器坐标准确性
#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif

#pragma comment(lib, "Winmm.lib")

MAA_CTRL_UNIT_NS_BEGIN

std::atomic<bool> MessageInput::hook_block_mouse_{ false };
std::atomic<MessageInput*> MessageInput::s_active_instance_{ nullptr };

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
    restore_pos();
    unblock_input();
    tracking_exit_ = true;
    if (tracking_thread_.joinable()) {
        tracking_thread_.join();
    }
}

void MessageInput::send_activate()
{
    bool use_post = (config_.mode == Mode::PostMessage);
    ::MaaNS::CtrlUnitNs::send_activate_message(hwnd_, use_post);
}

bool MessageInput::send_or_post_w(UINT message, WPARAM wParam, LPARAM lParam)
{
    bool success = false;

    if (config_.mode == Mode::PostMessage) {
        success = PostMessageW(hwnd_, message, wParam, lParam) != 0;
    }
    else {
        SendMessageW(hwnd_, message, wParam, lParam);
        success = true; // SendMessage 总是返回，除非窗口句柄无效
    }

    if (!success) {
        DWORD error = GetLastError();
        LogError << "Failed to" << config_.mode << VAR(message) << VAR(wParam) << VAR(lParam) << VAR(error);
    }

    return success;
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

    if (!SetWindowPos(hwnd_, nullptr, saved_window_rect_.left, saved_window_rect_.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)) {
        LogError << "SetWindowPos failed during restore" << VAR(hwnd_) << VAR(GetLastError());
    }
    window_pos_saved_ = false;
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

void MessageInput::restore_pos()
{
    if (config_.with_cursor_pos) {
        restore_cursor_pos();
    }
    else if (config_.with_window_pos) {
        tracking_active_ = false;
        s_active_instance_ = nullptr;
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

    RECT current_rect;
    if (!GetWindowRect(hwnd_, &current_rect)) {
        LogError << "GetWindowRect failed" << VAR(hwnd_) << VAR(GetLastError());
        return false;
    }

    // 锚点算法：使用客户区起始位置与窗口真实坐标间的偏移量，避免由于异步调用的延迟导致的累积拉扯误差
    int border_x = pt.x - current_rect.left;
    int border_y = pt.y - current_rect.top;

    int new_left = cursor_pos.x - x - border_x;
    int new_top = cursor_pos.y - y - border_y;

    if (!SetWindowPos(hwnd_, nullptr, new_left, new_top, 0, 0,
                      SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS)) {
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
        tracking_x_ = x;
        tracking_y_ = y;
        s_active_instance_ = this;
        tracking_active_ = true;

        move_window_to_align_cursor(x, y);
    }
    return MAKELPARAM(x, y);
}

// WH_MOUSE_LL 钩子回调：累加硬件鼠标位移 delta 并拦截，由追踪线程 60fps 批量释放
LRESULT CALLBACK MessageInput::MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && wParam == WM_MOUSEMOVE) {
        // 在关键初始化窗口期间，直接吞掉所有鼠标移动
        if (hook_block_mouse_) {
            return 1;
        }

        MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;

        // 跳过注入的合成事件（如我们自己的 SetCursorPos），只处理真实硬件输入
        if (pMouse->flags & LLMHF_INJECTED) {
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }

        MessageInput* inst = s_active_instance_.load(std::memory_order_acquire);
        if (inst && inst->tracking_active_) {
            // pt 是系统根据 "当前光标位置 + 硬件原始delta" 计算出的目标位置
            // 光标被我们冻住了，所以 delta = pt - 当前冻住的光标位置
            POINT cursor;
            GetCursorPos(&cursor);
            int dx = pMouse->pt.x - cursor.x;
            int dy = pMouse->pt.y - cursor.y;

            // 使用 fetch_add 累加每次的增量，不丢失任何中间移动
            inst->pending_mouse_x_.fetch_add(dx, std::memory_order_relaxed);
            inst->pending_mouse_y_.fetch_add(dy, std::memory_order_relaxed);
            inst->has_pending_mouse_.store(true, std::memory_order_release);

            // 拦截原始硬件鼠标移动（稍后由追踪线程通过 SetCursorPos 一次性释放累积量）
            return 1;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// ======================== 目标进程挂起/恢复 ========================

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
    }
}

void MessageInput::close_target_process()
{
    if (!target_process_handle_) {
        return;
    }
    CloseHandle(target_process_handle_);
    target_process_handle_ = nullptr;
}

// 动态解析 ntdll 未文档化函数（仅解析一次，线程安全由 static 局部变量保证）
template <typename Fn>
static Fn resolve_nt_function(const char* name)
{
    static Fn func = nullptr;
    static bool resolved = false;
    if (!resolved) {
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        if (ntdll) {
            func = reinterpret_cast<Fn>(GetProcAddress(ntdll, name));
        }
        if (!func) {
            LogWarn << "Failed to resolve ntdll function" << VAR(name);
        }
        resolved = true;
    }
    return func;
}

void MessageInput::suspend_target_process()
{
    if (!target_process_handle_) {
        return;
    }

    using NtSuspendProcessFn = LONG(NTAPI*)(HANDLE);
    auto fn = resolve_nt_function<NtSuspendProcessFn>("NtSuspendProcess");
    if (fn) {
        fn(target_process_handle_);
    }
}

void MessageInput::resume_target_process()
{
    if (!target_process_handle_) {
        return;
    }

    using NtResumeProcessFn = LONG(NTAPI*)(HANDLE);
    auto fn = resolve_nt_function<NtResumeProcessFn>("NtResumeProcess");
    if (fn) {
        fn(target_process_handle_);
    }
}

// ======================== 追踪线程 ========================

void MessageInput::process_pending_mouse_frame()
{
    has_pending_mouse_.store(false, std::memory_order_relaxed);

    // 原子读取并清零累积的 delta（exchange 保证不丢失并发写入）
    int dx = pending_mouse_x_.exchange(0, std::memory_order_relaxed);
    int dy = pending_mouse_y_.exchange(0, std::memory_order_relaxed);
    int tx = tracking_x_.load(std::memory_order_relaxed);
    int ty = tracking_y_.load(std::memory_order_relaxed);

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
    int new_top  = my - ty - border_y;

    // 1. 挂起目标进程，使其看不到中间态
    suspend_target_process();

    // 2. 移动窗口（SWP_ASYNCWINDOWPOS 避免阻塞 + SWP_NOSENDCHANGING 跳过同步通知）
    SetWindowPos(hwnd_, nullptr, new_left, new_top, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_ASYNCWINDOWPOS);

    // 3. 释放光标到累积后的真实目标位置
    SetCursorPos(mx, my);

    // 4. 恢复目标进程（它醒来时看到的窗口和光标已完美对齐）
    resume_target_process();
}

void MessageInput::tracking_thread_func()
{
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    // 将系统定时器精度提升到 1ms，确保 Sleep/MsgWait 精度
    timeBeginPeriod(1);
    OnScopeLeave([this]() {
        close_target_process();
        timeEndPeriod(1);
    });

    // 预先打开目标进程句柄（用于挂起/恢复）
    open_target_process();

    HHOOK hHook = SetWindowsHookExW(WH_MOUSE_LL, MouseHookProc, GetModuleHandleW(NULL), 0);
    if (!hHook) {
        LogError << "SetWindowsHookExW failed, tracking disabled" << VAR(GetLastError());
        return;
    }
    OnScopeLeave([&]() { UnhookWindowsHookEx(hHook); });

    // 60fps 节流：每帧间隔 ~16.67ms
    using clock = std::chrono::steady_clock;
    auto last_frame = clock::now();
    const auto frame_interval = std::chrono::microseconds(16667);

    MSG msg;
    while (!tracking_exit_) {
        // 消息泵：WH_MOUSE_LL 钩子回调在此线程上被系统调度，必须保持消息循环活跃
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

        if (tracking_active_ && has_pending_mouse_.load(std::memory_order_acquire) && frame_ready) {
            process_pending_mouse_frame();
            last_frame = now;
        }
    }
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

std::pair<int, int> MessageInput::get_target_pos() const
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

    send_activate();

    check_and_block_input();

    save_pos();

    // 准备位置并发送 MOVE 消息
    LPARAM lParam = prepare_mouse_position(x, y);

    if (!send_or_post_w(move_info.message, move_info.w_param, lParam)) {
        restore_pos();
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // 发送 DOWN 消息
    if (!send_or_post_w(down_info.message, down_info.w_param, lParam)) {
        restore_pos();
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

    // 准备位置并发送 MOVE 消息
    LPARAM lParam = prepare_mouse_position(x, y);

    if (!send_or_post_w(msg_info.message, msg_info.w_param, lParam)) {
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

    send_activate();

    OnScopeLeave([this]() { unblock_input(); });

    MouseMessageInfo msg_info;
    if (!contact_to_mouse_up_message(contact, msg_info)) {
        LogError << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << "contact out of range"
                 << VAR(contact);
        return false;
    }

    auto target_pos = get_target_pos();
    if (!send_or_post_w(msg_info.message, msg_info.w_param, MAKELPARAM(target_pos.first, target_pos.second))) {
        restore_pos();
        return false;
    }

    // touch_up 时恢复位置（与 touch_down 配对）
    restore_pos();

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

    send_activate();

    bool success = true;

    // 文本输入仅发送 WM_CHAR
    for (const auto ch : to_u16(text)) {
        success &= send_or_post_w(WM_CHAR, static_cast<WPARAM>(ch), 0);
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

    send_activate();

    LPARAM lParam = make_keydown_lparam(key);
    return send_or_post_w(WM_KEYDOWN, static_cast<WPARAM>(key), lParam);
}

bool MessageInput::key_up(int key)
{
    LogInfo << VAR(config_.mode) << VAR(key);

    if (!hwnd_) {
        LogError << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << "hwnd_ is nullptr";
        return false;
    }

    send_activate();

    LPARAM lParam = make_keyup_lparam(key);
    return send_or_post_w(WM_KEYUP, static_cast<WPARAM>(key), lParam);
}

bool MessageInput::scroll(int dx, int dy)
{
    LogInfo << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << VAR(dx) << VAR(dy);

    if (!hwnd_) {
        LogError << VAR(config_.mode) << VAR(config_.with_cursor_pos) << VAR(config_.with_window_pos) << "hwnd_ is nullptr";
        return false;
    }

    send_activate();

    check_and_block_input();
    OnScopeLeave([this]() { unblock_input(); });

    auto target_pos = get_target_pos();

    save_pos();

    // prepare_mouse_position 用于移动光标/窗口（副作用），但 WM_MOUSEWHEEL 的 lParam 需要屏幕坐标
    prepare_mouse_position(target_pos.first, target_pos.second);
    POINT screen_pos = client_to_screen(target_pos.first, target_pos.second);
    LPARAM lParam = MAKELPARAM(screen_pos.x, screen_pos.y);

    bool success = true;

    if (dy != 0) {
        WPARAM wParam = MAKEWPARAM(0, static_cast<short>(dy));
        success &= send_or_post_w(WM_MOUSEWHEEL, wParam, lParam);
    }

    if (dx != 0) {
        WPARAM wParam = MAKEWPARAM(0, static_cast<short>(dx));
        success &= send_or_post_w(WM_MOUSEHWHEEL, wParam, lParam);
    }

    restore_pos();

    return success;
}

MAA_CTRL_UNIT_NS_END
