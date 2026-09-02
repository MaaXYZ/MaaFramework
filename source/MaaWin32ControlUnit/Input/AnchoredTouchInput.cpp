#include "AnchoredTouchInput.h"

#include <format>
#include <vector>

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

namespace
{

// 接触点 id 必须是小整数，实测用 0xA000 这类大值会被合成设备整帧拒绝
constexpr uint32_t kAnchorPointerId = 1;
constexpr uint32_t kContactPointerIdBase = 2;

// 上层可用的接触点数量。锚点自己要占掉一个名额，创建合成设备时需要一并算上，
// 否则上层用满多点触控时最后一个接触点会因为超出设备容量而被拒绝
constexpr ULONG kMaxTouchPoints = 10;
constexpr ULONG kMaxContacts = kMaxTouchPoints + 1;

constexpr int kAnchorSize = 4;
constexpr int kAnchorMargin = 8;

// 分层窗口 alpha 为 0 时命中测试会穿透，锚点将吃不到主指针，所以取最小的可见值
constexpr BYTE kAnchorAlpha = 1;

// 借用目标窗口期间同样降到最低 alpha，肉眼几乎看不到画面变化，但命中判定仍然有效
constexpr BYTE kDimAlpha = 1;

// 注入帧间隔。实测超过约 400ms 不提交新帧，系统会回收接触点
constexpr int kTickMs = 12;

// 空闲多久后停止注入线程。inactive() 是主要的释放入口，这里只作兜底
constexpr int kIdleShutdownMs = 60 * 1000;

// touch_move 等待新坐标被提交所需的帧数。一帧即可确保当前坐标进入注入序列，
// 多等一帧会让每次 touch_move 平均阻塞一个额外的 tick，而上层按距离把一次滑动拆成数十步，
// 该开销会成倍放大：实测 200ms 的滑动一度耗时 1244ms
constexpr int kWaitFrames = 1;
constexpr auto kWaitTimeout = std::chrono::milliseconds(2000);

constexpr UINT32 kDownFlags = POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT | POINTER_FLAG_CONFIDENCE;
constexpr UINT32 kUpdateFlags = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT | POINTER_FLAG_CONFIDENCE;
constexpr UINT32 kUpFlags = POINTER_FLAG_UP;

using CreateSyntheticPointerDeviceFunc = HSYNTHETICPOINTERDEVICE(WINAPI*)(POINTER_INPUT_TYPE, ULONG, POINTER_FEEDBACK_MODE);
using InjectSyntheticPointerInputFunc = BOOL(WINAPI*)(HSYNTHETICPOINTERDEVICE, const POINTER_TYPE_INFO*, UINT32);
using DestroySyntheticPointerDeviceFunc = void(WINAPI*)(HSYNTHETICPOINTERDEVICE);

std::once_flag g_api_once;
CreateSyntheticPointerDeviceFunc g_create_device = nullptr;
InjectSyntheticPointerInputFunc g_inject_input = nullptr;
DestroySyntheticPointerDeviceFunc g_destroy_device = nullptr;

POINTER_TYPE_INFO make_touch_info(uint32_t id, POINT point, UINT32 flags)
{
    POINTER_TYPE_INFO info = { };
    info.type = PT_TOUCH;
    info.touchInfo.pointerInfo.pointerType = PT_TOUCH;
    info.touchInfo.pointerInfo.pointerId = id;
    info.touchInfo.pointerInfo.pointerFlags = flags;
    info.touchInfo.pointerInfo.ptPixelLocation = point;
    info.touchInfo.touchFlags = TOUCH_FLAG_NONE;
    info.touchInfo.touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
    info.touchInfo.rcContact = { point.x - 2, point.y - 2, point.x + 2, point.y + 2 };
    info.touchInfo.rcContactRaw = info.touchInfo.rcContact;
    info.touchInfo.orientation = 90;
    info.touchInfo.pressure = 32000;
    return info;
}

}

bool AnchoredTouchInput::load_injection_api()
{
    std::call_once(g_api_once, [] {
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        if (!user32) {
            return;
        }
        g_create_device = reinterpret_cast<CreateSyntheticPointerDeviceFunc>(GetProcAddress(user32, "CreateSyntheticPointerDevice"));
        g_inject_input = reinterpret_cast<InjectSyntheticPointerInputFunc>(GetProcAddress(user32, "InjectSyntheticPointerInput"));
        g_destroy_device = reinterpret_cast<DestroySyntheticPointerDeviceFunc>(GetProcAddress(user32, "DestroySyntheticPointerDevice"));
    });

    return g_create_device && g_inject_input && g_destroy_device;
}

AnchoredTouchInput::AnchoredTouchInput(HWND hwnd)
    : hwnd_(hwnd)
{
    class_name_ = std::format(L"MaaAnchoredTouch_{}_{}", GetCurrentProcessId(), reinterpret_cast<uintptr_t>(this));
}

AnchoredTouchInput::~AnchoredTouchInput()
{
    stop_worker();
}

MaaControllerFeature AnchoredTouchInput::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick;
}

// get_features() 返回 MaaControllerFeature_UseMouseDownAndUpInsteadOfClick，
// ControllerAgent 会用 touch_down / touch_move / touch_up 组合出 click 与 swipe，
// 因此下面两个接口不会被调用到

bool AnchoredTouchInput::click(int x, int y)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_up instead"
             << VAR(x) << VAR(y);
    return false;
}

bool AnchoredTouchInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_move/touch_up instead"
             << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool AnchoredTouchInput::touch_down(int contact, int x, int y, [[maybe_unused]] int pressure)
{
    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR_VOIDP(hwnd_);

    if (!ensure_worker()) {
        return false;
    }

    // 最小化窗口的客户区不在屏幕上，ClientToScreen 会给出屏幕外的坐标，提升也救不回来。
    // 截图方式选了带伪最小化的那两种时，窗口早已被转成非最小化状态，这里不会触发；
    // 选其他截图方式时截图本身也无法工作，此处拒绝并报错
    if (IsIconic(hwnd_)) {
        LogError << "the target window is minimized" << VAR_VOIDP(hwnd_);
        return false;
    }

    POINT point = { };
    if (!to_screen(x, y, point)) {
        release_window_if_idle();
        return false;
    }

    // 目标点打不中目标窗口就不能注入，否则输入会落到遮挡它的那个窗口上
    if (!ensure_hittable(point)) {
        release_window_if_idle();
        return false;
    }

    std::unique_lock lock(mutex_);
    last_activity_ = std::chrono::steady_clock::now();

    auto& state = contacts_[contact];
    state.screen_pos = point;
    state.pending_down = true;
    state.pending_up = false;
    state.active = false;

    if (!wait_for_contact_active(lock, contact)) {
        // 接触点没能按下，清掉它，避免目标窗口停在提升状态
        contacts_.erase(contact);
        lock.unlock();
        release_window_if_idle();
        return false;
    }

    return true;
}

bool AnchoredTouchInput::touch_move(int contact, int x, int y, [[maybe_unused]] int pressure)
{
    if (!ensure_worker()) {
        return false;
    }

    POINT point = { };
    if (!to_screen(x, y, point)) {
        return false;
    }

    std::unique_lock lock(mutex_);
    last_activity_ = std::chrono::steady_clock::now();

    auto it = contacts_.find(contact);
    if (it == contacts_.end()) {
        LogError << "contact is not down" << VAR(contact);
        return false;
    }

    it->second.screen_pos = point;

    return wait_for_frames(lock, kWaitFrames);
}

bool AnchoredTouchInput::touch_up(int contact)
{
    LogInfo << VAR(contact) << VAR_VOIDP(hwnd_);

    if (!ready_ || setup_failed_) {
        LogError << "worker is not running" << VAR(contact);
        return false;
    }

    std::unique_lock lock(mutex_);
    last_activity_ = std::chrono::steady_clock::now();

    auto it = contacts_.find(contact);
    if (it == contacts_.end()) {
        LogError << "contact is not down" << VAR(contact);
        lock.unlock();
        release_window_if_idle();
        return false;
    }

    it->second.pending_up = true;

    bool ok = wait_for_contact_released(lock, contact);
    lock.unlock();

    // 必须等所有接触点都抬起后再归还窗口，否则剩下的接触点会在触控序列中途丢失命中
    release_window_if_idle();

    return ok;
}

bool AnchoredTouchInput::click_key(int key)
{
    LogError << "AnchoredTouch only implements mouse input, please select another method for keyboard" << VAR(key);
    return false;
}

bool AnchoredTouchInput::input_text(const std::string& text)
{
    LogError << "AnchoredTouch only implements mouse input, please select another method for keyboard" << VAR(text);
    return false;
}

bool AnchoredTouchInput::key_down(int key)
{
    LogError << "AnchoredTouch only implements mouse input, please select another method for keyboard" << VAR(key);
    return false;
}

bool AnchoredTouchInput::key_up(int key)
{
    LogError << "AnchoredTouch only implements mouse input, please select another method for keyboard" << VAR(key);
    return false;
}

bool AnchoredTouchInput::scroll(int dx, int dy)
{
    LogError << "AnchoredTouch does not implement scroll" << VAR(dx) << VAR(dy);
    return false;
}

void AnchoredTouchInput::inactive()
{
    stop_worker();
}

bool AnchoredTouchInput::ensure_worker()
{
    if (setup_failed_) {
        return false;
    }

    if (ready_) {
        return true;
    }

    if (!hwnd_) {
        LogError << "hwnd_ is nullptr";
        return false;
    }

    if (!load_injection_api()) {
        LogError << "synthetic pointer injection is unavailable, Windows 10 1809 or later is required";
        setup_failed_ = true;
        return false;
    }

    // 上一轮线程已退出但尚未回收
    if (worker_.joinable()) {
        worker_.join();
    }

    std::unique_lock lock(mutex_);

    running_ = true;
    frame_count_ = 0;
    anchor_down_ = false;
    last_activity_ = std::chrono::steady_clock::now();
    contacts_.clear();
    worker_ = std::thread(&AnchoredTouchInput::worker_main, this);

    cv_.wait_for(lock, kWaitTimeout, [this] { return ready_.load(); });

    return ready_ && !setup_failed_;
}

void AnchoredTouchInput::stop_worker()
{
    running_ = false;

    if (worker_.joinable()) {
        worker_.join();
    }

    ready_ = false;

    unprepare_window();

    std::lock_guard lock(mutex_);
    contacts_.clear();
}

void AnchoredTouchInput::worker_main()
{
#ifndef MAA_WIN32_COMPATIBLE
    // DPI 感知是线程级的，Win32ControlUnitMgr::connect() 里的设置不会传递到这里。
    // 注入用的是物理像素，注入线程也必须拿到物理像素的窗口矩形与屏幕尺寸。
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#endif

    if (!worker_setup()) {
        setup_failed_ = true;
        ready_ = true;
        cv_.notify_all();
        worker_teardown();
        return;
    }

    ready_ = true;
    cv_.notify_all();

    auto next_tick = std::chrono::steady_clock::now();

    while (running_) {
        MSG msg = { };
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        auto now = std::chrono::steady_clock::now();
        if (now < next_tick) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        next_tick = now + std::chrono::milliseconds(kTickMs);
        follow_target_window();
        worker_tick();

        // 注入后立刻再泵一次消息，让锚点窗口尽快处理掉本帧的指针消息。
        // 锚点窗口过程一旦响应不及时，系统会走默认路径把主指针提升为鼠标事件。
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        bool idle = false;
        {
            std::lock_guard lock(mutex_);
            auto idle_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_activity_).count();
            idle = contacts_.empty() && !anchor_down_ && idle_ms >= kIdleShutdownMs;
        }

        if (idle) {
            LogInfo << "anchored touch is idle, stopping worker";
            running_ = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    worker_teardown();

    // 空闲自停时 stop_worker() 不会被调用，目标窗口的样式改动得由这里还原，
    // 否则 WS_EX_LAYERED 会一直挂到控制单元析构
    unprepare_window();

    ready_ = false;
    cv_.notify_all();
}

bool AnchoredTouchInput::worker_setup()
{
    WNDCLASSEXW wc = { };
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &AnchoredTouchInput::anchor_wnd_proc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = class_name_.c_str();

    if (RegisterClassExW(&wc) == 0) {
        LogError << "RegisterClassExW failed" << VAR(GetLastError());
        return false;
    }

    POINT origin = compute_anchor_origin();

    HWND anchor = CreateWindowExW(
        WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_LAYERED,
        class_name_.c_str(),
        L"",
        WS_POPUP,
        origin.x,
        origin.y,
        kAnchorSize,
        kAnchorSize,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr);

    anchor_hwnd_ = anchor;

    if (!anchor) {
        LogError << "CreateWindowExW failed" << VAR(GetLastError());
        return false;
    }

    if (!SetLayeredWindowAttributes(anchor, 0, kAnchorAlpha, LWA_ALPHA)) {
        LogError << "SetLayeredWindowAttributes failed" << VAR(GetLastError());
        return false;
    }

    ShowWindow(anchor, SW_SHOWNOACTIVATE);

    // 读不到锚点矩形就没有可信的注入位置，anchor_pos_ 会停在 (0, 0)，主指针随之落到屏幕左上角
    RECT anchor_rect = { };
    if (!GetWindowRect(anchor, &anchor_rect)) {
        LogError << "GetWindowRect failed" << VAR_VOIDP(anchor) << VAR(GetLastError());
        return false;
    }

    anchor_pos_ = { (anchor_rect.left + anchor_rect.right) / 2, (anchor_rect.top + anchor_rect.bottom) / 2 };

    auto device = g_create_device(PT_TOUCH, kMaxContacts, POINTER_FEEDBACK_NONE);
    if (!device) {
        LogError << "CreateSyntheticPointerDevice failed" << VAR(GetLastError());
        return false;
    }
    device_ = device;

    GetWindowRect(hwnd_, &last_target_rect_);

    LogInfo << "anchored touch is ready" << VAR_VOIDP(anchor) << VAR(anchor_pos_.x) << VAR(anchor_pos_.y);

    return true;
}

void AnchoredTouchInput::worker_teardown()
{
    if (device_) {
        if (anchor_down_) {
            auto info = make_touch_info(kAnchorPointerId, anchor_pos_, kUpFlags);
            g_inject_input(static_cast<HSYNTHETICPOINTERDEVICE>(device_), &info, 1);
            anchor_down_ = false;
        }

        g_destroy_device(static_cast<HSYNTHETICPOINTERDEVICE>(device_));
        device_ = nullptr;
    }

    if (HWND anchor = anchor_hwnd_.exchange(nullptr)) {
        DestroyWindow(anchor);
    }

    UnregisterClassW(class_name_.c_str(), GetModuleHandleW(nullptr));
}

void AnchoredTouchInput::follow_target_window()
{
    RECT rect = { };
    if (!GetWindowRect(hwnd_, &rect)) {
        return;
    }

    if (rect.left == last_target_rect_.left && rect.top == last_target_rect_.top && rect.right == last_target_rect_.right
        && rect.bottom == last_target_rect_.bottom) {
        return;
    }

    last_target_rect_ = rect;

    POINT origin = compute_anchor_origin();

    // 锚点没挪成还照着新位置注入的话，主指针会落在锚点窗口之外，
    // 保护前提失效，可能打到别的窗口上并重新引发鼠标提升。
    // 失败时锚点窗口与 anchor_pos_ 一起留在原处，两边仍然自洽
    if (!SetWindowPos(
            anchor_hwnd_.load(),
            HWND_TOPMOST,
            origin.x,
            origin.y,
            kAnchorSize,
            kAnchorSize,
            SWP_NOACTIVATE | SWP_NOOWNERZORDER)) {
        LogError << "failed to move the anchor window" << VAR(origin.x) << VAR(origin.y) << VAR(GetLastError());
        return;
    }

    anchor_pos_ = { origin.x + kAnchorSize / 2, origin.y + kAnchorSize / 2 };
}

void AnchoredTouchInput::worker_tick()
{
    std::vector<POINTER_TYPE_INFO> frame;

    // 本帧打算做的状态变更。注入成功之前一律不落实，否则注入失败时
    // 等待中的 touch_down / touch_up 会被当作成功唤醒，上层以为操作生效了
    bool commit_anchor_down = false;
    bool commit_anchor_up = false;
    std::vector<int> commit_activated;
    std::vector<int> commit_released;

    {
        std::lock_guard lock(mutex_);

        bool want_anchor = !contacts_.empty();

        if (want_anchor && !anchor_down_) {
            frame.emplace_back(make_touch_info(kAnchorPointerId, anchor_pos_, kDownFlags));
            commit_anchor_down = true;
        }
        else if (anchor_down_) {
            if (want_anchor) {
                frame.emplace_back(make_touch_info(kAnchorPointerId, anchor_pos_, kUpdateFlags));
            }
            else {
                // 没有待处理的接触点就立刻松开锚点，不让触控状态在任务之间残留
                frame.emplace_back(make_touch_info(kAnchorPointerId, anchor_pos_, kUpFlags));
                commit_anchor_up = true;
            }
        }

        for (auto& [contact, state] : contacts_) {
            uint32_t id = kContactPointerIdBase + static_cast<uint32_t>(contact);

            if (state.pending_down) {
                // 锚点必须先单独成帧按下并取得主指针身份，操作点才能作为非主指针注入，
                // 同一帧提交会让主指针归属落到操作点上，表现为光标被抢走。
                // anchor_down_ 此处仍是上一帧的状态，锚点本帧才按下时它为假
                if (!anchor_down_) {
                    continue;
                }

                frame.emplace_back(make_touch_info(id, state.screen_pos, kDownFlags));
                commit_activated.emplace_back(contact);
            }
            else if (state.pending_up) {
                frame.emplace_back(make_touch_info(id, state.screen_pos, kUpFlags));
                commit_released.emplace_back(contact);
            }
            else if (state.active) {
                frame.emplace_back(make_touch_info(id, state.screen_pos, kUpdateFlags));
            }
        }
    }

    bool injected = true;
    if (!frame.empty()) {
        SetLastError(0);
        injected = g_inject_input(static_cast<HSYNTHETICPOINTERDEVICE>(device_), frame.data(), static_cast<UINT32>(frame.size())) != FALSE;

        if (!injected) {
            // 整帧是原子的，其中任何一个接触点非法都会让本帧全部失败
            LogError << "InjectSyntheticPointerInput failed" << VAR(GetLastError()) << VAR(frame.size());
        }
    }

    if (!injected) {
        // 本帧没有到达系统，本地状态保持不变，下一帧原样重试。
        // frame_count_ 只统计成功提交的帧，持续失败会让等待方超时失败
        return;
    }

    {
        std::lock_guard lock(mutex_);

        if (commit_anchor_down) {
            anchor_down_ = true;
        }
        else if (commit_anchor_up) {
            anchor_down_ = false;
        }

        for (int contact : commit_activated) {
            auto it = contacts_.find(contact);
            if (it == contacts_.end()) {
                continue;
            }
            it->second.pending_down = false;
            it->second.active = true;
        }

        for (int contact : commit_released) {
            contacts_.erase(contact);
        }

        ++frame_count_;
    }

    cv_.notify_all();
}

bool AnchoredTouchInput::wait_for_contact_active(std::unique_lock<std::mutex>& lock, int contact)
{
    bool done = cv_.wait_for(lock, kWaitTimeout, [this, contact] {
        auto it = contacts_.find(contact);
        return !running_ || (it != contacts_.end() && it->second.active);
    });

    if (!done) {
        LogError << "timed out waiting for contact down" << VAR(contact);
        return false;
    }

    return running_;
}

bool AnchoredTouchInput::wait_for_contact_released(std::unique_lock<std::mutex>& lock, int contact)
{
    bool done = cv_.wait_for(lock, kWaitTimeout, [this, contact] { return !running_ || !contacts_.contains(contact); });

    if (!done) {
        LogError << "timed out waiting for contact up" << VAR(contact);
        return false;
    }

    return running_;
}

bool AnchoredTouchInput::wait_for_frames(std::unique_lock<std::mutex>& lock, int frames)
{
    uint64_t target = frame_count_ + static_cast<uint64_t>(frames);

    bool done = cv_.wait_for(lock, kWaitTimeout, [this, target] { return !running_ || frame_count_ >= target; });

    if (!done) {
        LogError << "timed out waiting for injection frame" << VAR(target) << VAR(frame_count_);
        return false;
    }

    return running_;
}

bool AnchoredTouchInput::to_screen(int x, int y, POINT& out) const
{
    POINT point = { x, y };
    if (!ClientToScreen(hwnd_, &point)) {
        LogError << "ClientToScreen failed" << VAR(GetLastError()) << VAR(x) << VAR(y);
        return false;
    }

    out = point;
    return true;
}

POINT AnchoredTouchInput::compute_anchor_origin() const
{
    int screen_left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int screen_top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int screen_right = screen_left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screen_bottom = screen_top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

    const POINT candidates[] = {
        { screen_left + kAnchorMargin, screen_top + kAnchorMargin },
        { screen_right - kAnchorSize - kAnchorMargin, screen_top + kAnchorMargin },
        { screen_left + kAnchorMargin, screen_bottom - kAnchorSize - kAnchorMargin },
        { screen_right - kAnchorSize - kAnchorMargin, screen_bottom - kAnchorSize - kAnchorMargin },
    };

    RECT window_rect = { };
    if (!GetWindowRect(hwnd_, &window_rect)) {
        return candidates[0];
    }

    // 锚点必须避开目标窗口。曾把它放在目标窗口的标题栏上，锚点的 Z 序因此和目标窗口绑在一起，
    // 提升目标窗口时会盖住锚点，主指针身份随之转移到操作点，表现为偶发抢鼠标
    for (const auto& candidate : candidates) {
        RECT rect = { candidate.x, candidate.y, candidate.x + kAnchorSize, candidate.y + kAnchorSize };
        bool intersects = rect.left < window_rect.right && rect.right > window_rect.left && rect.top < window_rect.bottom
                          && rect.bottom > window_rect.top;

        if (!intersects) {
            return candidate;
        }
    }

    // 目标窗口铺满整个虚拟屏幕时四角都躲不开，此时靠锚点自身的 topmost 属性压在目标窗口之上
    return candidates[0];
}

bool AnchoredTouchInput::is_occluded(POINT screen) const
{
    HWND hit = WindowFromPoint(screen);
    HWND root = GetAncestor(hit, GA_ROOT);

    return hit != hwnd_ && root != hwnd_ && !IsChild(hwnd_, hit);
}

bool AnchoredTouchInput::anchor_covers(POINT screen) const
{
    HWND anchor = anchor_hwnd_.load();
    if (!anchor) {
        return false;
    }

    RECT rect = { };
    if (!GetWindowRect(anchor, &rect)) {
        return false;
    }

    return PtInRect(&rect, screen) != FALSE;
}

bool AnchoredTouchInput::prepare_window()
{
    // GetWindowLongPtrW 返回 0 也可能是合法值，需要靠 GetLastError 区分
    SetLastError(0);
    LONG_PTR exstyle = GetWindowLongPtrW(hwnd_, GWL_EXSTYLE);
    if (exstyle == 0 && GetLastError() != 0) {
        LogError << "GetWindowLongPtrW failed" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
        return false;
    }

    if (!window_prepared_) {
        window_prepared_ = true;
        window_prepare_ok_ = true;
        original_layered_ = (exstyle & WS_EX_LAYERED) != 0;

        if (original_layered_) {
            // GetLayeredWindowAttributes 只在窗口的分层属性由 SetLayeredWindowAttributes 设置时才成功。
            // 失败说明目标窗口走的是 UpdateLayeredWindow，一旦被 SetLayeredWindowAttributes 碰过，
            // 它后续的 UpdateLayeredWindow 会一直失败直到样式位被重设，因此这种窗口不能提升。
            // 这里只做能力判定，属性值由 dim_window() 在每次借用前读取
            COLORREF color_key = 0;
            BYTE alpha = 0;
            DWORD layered_flags = 0;
            if (!GetLayeredWindowAttributes(hwnd_, &color_key, &alpha, &layered_flags)) {
                LogError << "target window is layered via UpdateLayeredWindow, refuse to raise it" << VAR_VOIDP(hwnd_)
                         << VAR(GetLastError());
                window_prepare_ok_ = false;
                return false;
            }
        }
        else {
            // 文档规定 WS_EX_LAYERED 不能用于 CS_OWNDC / CS_CLASSDC 窗口类，但该限制并不总是成立：
            // Unity 的 UnityWndClass 带 CS_OWNDC，实测分层与提升都正常。
            // 因此这里只留一条线索，是否可用由后面几个 API 的实际返回值决定
            DWORD class_style = static_cast<DWORD>(GetClassLongPtrW(hwnd_, GCL_STYLE));
            if ((class_style & (CS_OWNDC | CS_CLASSDC)) != 0) {
                LogWarn << "target window class uses CS_OWNDC or CS_CLASSDC, WS_EX_LAYERED may not work" << VAR_VOIDP(hwnd_)
                        << VAR(class_style);
            }
        }
    }

    if (!window_prepare_ok_) {
        return false;
    }

    // 分层样式的归属只看当前状态，不看首次准备时读到的值。首次准备若发生在截图侧伪最小化期间，
    // 读到的分层样式是对方挂的，对方还原后它就不存在了，沿用那一次的结论会让后面的
    // SetLayeredWindowAttributes 全部失败。目标程序自己（切换窗口模式、DPI 变化）也随时可能把它重设掉，
    // 一旦丢失又照旧提升，目标窗口会以完全不透明的状态被弹到最前，所以每次提升前都要重新确认它还在。
    // 反过来，窗口当前已是分层的就不动样式，只借用它的不透明度，也不记为我们的债务
    if ((exstyle & WS_EX_LAYERED) != 0) {
        return true;
    }

    SetLastError(0);
    if (SetWindowLongPtrW(hwnd_, GWL_EXSTYLE, exstyle | WS_EX_LAYERED) == 0 && GetLastError() != 0) {
        LogError << "SetWindowLongPtrW failed" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
        return false;
    }

    // 这是我们新建的分层状态，必须写明确的初值。original_* 记的是上一次借用时读到的属性，
    // 那一次若发生在截图侧伪最小化期间，记下的不透明度会是 0，沿用它会让目标窗口整个不可见
    if (!SetLayeredWindowAttributes(hwnd_, 0, 255, LWA_ALPHA)) {
        LogError << "SetLayeredWindowAttributes failed" << VAR_VOIDP(hwnd_) << VAR(GetLastError());

        // 回滚也可能失败，那样分层样式就留在了目标窗口上，必须记成待清理的债务
        SetLastError(0);
        if (SetWindowLongPtrW(hwnd_, GWL_EXSTYLE, exstyle) == 0 && GetLastError() != 0) {
            LogError << "failed to roll back WS_EX_LAYERED" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
            layered_applied_ = true;
        }

        return false;
    }

    layered_applied_ = true;
    return true;
}

void AnchoredTouchInput::begin_borrow()
{
    if (borrowed_) {
        return;
    }

    borrowed_ = true;
    prev_sibling_ = GetWindow(hwnd_, GW_HWNDPREV);

    // 置顶状态与分层属性一样按次读取，跨借用沿用会写回过期的值
    original_topmost_ = (GetWindowLongPtrW(hwnd_, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
}

bool AnchoredTouchInput::dim_window()
{
    if (dimmed_) {
        return true;
    }

    // 分层属性必须在每次借用前重新读取，不能沿用上一次借用时记下的值。
    // 截图侧的伪最小化会把目标窗口的不透明度设为 0，其还原又会改回 255，
    // 写回一份跨借用过期的值会让目标窗口停在错误的可见状态上
    if (!GetLayeredWindowAttributes(hwnd_, &original_color_key_, &original_alpha_, &original_layered_flags_)) {
        LogError << "failed to read the target window layered attributes" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
        return false;
    }

    // 保留原有的 color key 位，只把不透明度压到最低，避免借用期间改变目标窗口的透明色行为
    DWORD flags = LWA_ALPHA | (original_layered_flags_ & LWA_COLORKEY);

    if (!SetLayeredWindowAttributes(hwnd_, original_color_key_, kDimAlpha, flags)) {
        LogError << "failed to lower the target window alpha, refuse to make it visible" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
        return false;
    }

    mark_color_key_ = original_color_key_;
    mark_layered_flags_ = flags;

    dimmed_ = true;
    return true;
}

AnchoredTouchInput::WindowOwnership AnchoredTouchInput::check_borrow_mark() const
{
    SetLastError(0);
    LONG_PTR exstyle = GetWindowLongPtrW(hwnd_, GWL_EXSTYLE);
    if (exstyle == 0 && GetLastError() != 0) {
        return WindowOwnership::Unknown;
    }

    // 核对的只是还没交还的那几项。上一轮归还若只成功了一半，成功的那一项已经把窗口改回了原样，
    // 继续拿借用期间的完整预期去量，会把自己刚做的还原认成外部改动，剩下的一项从此再也不还
    if ((exstyle & WS_EX_LAYERED) == 0) {
        return WindowOwnership::Taken;
    }

    // 摘掉的 WS_EX_TRANSPARENT 又出现了，说明窗口已经被目标程序或其他模块重设过——例如截图侧的
    // 伪最小化还原会写回一份不含本次样式改动的扩展样式，此时再写回借用前的旧值会把窗口重新变成穿透
    if (transparent_suppressed_ && (exstyle & WS_EX_TRANSPARENT) != 0) {
        return WindowOwnership::Taken;
    }

    if (dimmed_) {
        COLORREF color_key = 0;
        BYTE alpha = 0;
        DWORD layered_flags = 0;
        if (!GetLayeredWindowAttributes(hwnd_, &color_key, &alpha, &layered_flags)) {
            // 读不到就无从判断，不能当作已被接管，保留待还原项等下一次机会
            return WindowOwnership::Unknown;
        }

        if (alpha != kDimAlpha || color_key != mark_color_key_ || layered_flags != mark_layered_flags_) {
            return WindowOwnership::Taken;
        }
    }

    return WindowOwnership::Ours;
}

bool AnchoredTouchInput::undim_window()
{
    if (!dimmed_) {
        return true;
    }

    if (!SetLayeredWindowAttributes(hwnd_, original_color_key_, original_alpha_, original_layered_flags_)) {
        LogError << "failed to restore the target window alpha" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
        return false;
    }

    dimmed_ = false;
    return true;
}

bool AnchoredTouchInput::suppress_transparent()
{
    SetLastError(0);
    LONG_PTR exstyle = GetWindowLongPtrW(hwnd_, GWL_EXSTYLE);
    if (exstyle == 0 && GetLastError() != 0) {
        LogError << "GetWindowLongPtrW failed" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
        return false;
    }

    if ((exstyle & WS_EX_TRANSPARENT) == 0) {
        return true;
    }

    // 分层窗口带 WS_EX_TRANSPARENT 时鼠标事件会穿透到下层窗口。合成指针是否同样穿透没有实测过，
    // 借用期间一律先摘掉该样式，两种情况下都能命中。
    // 截图侧的伪最小化正是靠这个样式让用户点不到隐形窗口的，还原时必须原样加回
    SetLastError(0);
    if (SetWindowLongPtrW(hwnd_, GWL_EXSTYLE, exstyle & ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT)) == 0 && GetLastError() != 0) {
        LogError << "failed to suppress WS_EX_TRANSPARENT" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
        return false;
    }

    transparent_suppressed_ = true;
    return true;
}

bool AnchoredTouchInput::restore_transparent()
{
    if (!transparent_suppressed_) {
        return true;
    }

    SetLastError(0);
    LONG_PTR exstyle = GetWindowLongPtrW(hwnd_, GWL_EXSTYLE);
    if (exstyle == 0 && GetLastError() != 0) {
        LogError << "GetWindowLongPtrW failed" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
        return false;
    }

    SetLastError(0);
    if (SetWindowLongPtrW(hwnd_, GWL_EXSTYLE, exstyle | WS_EX_TRANSPARENT) == 0 && GetLastError() != 0) {
        LogError << "failed to restore WS_EX_TRANSPARENT" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
        return false;
    }

    transparent_suppressed_ = false;
    return true;
}

bool AnchoredTouchInput::ensure_hittable(POINT screen)
{
    std::lock_guard lock(window_mutex_);

    if (!hwnd_) {
        return false;
    }

    // 锚点是 topmost 的，目标点落进它的矩形里就永远打不中目标窗口
    if (anchor_covers(screen)) {
        LogError << "the target point is covered by the anchor window" << VAR(screen.x) << VAR(screen.y);
        return false;
    }

    if (!is_occluded(screen)) {
        return true;
    }

    // 已经提升过还是打不中，说明有别的 topmost 窗口挡着，再提升一次也没用
    if (raised_) {
        LogError << "the target point is still occluded while the target window is raised" << VAR(screen.x) << VAR(screen.y);
        return false;
    }

    if (!prepare_window()) {
        return false;
    }

    begin_borrow();

    // 压不下不透明度就不提升。把目标窗口完全可见地弹到最前面，比这一次操作失败严重得多
    if (!dim_window() || !suppress_transparent()) {
        release_window_locked();
        return false;
    }

    // HWND_TOP 需要调用方具备 SetForegroundWindow 权限，后台进程用它不会真正改变 Z 序，只有 HWND_TOPMOST 有效
    if (!SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER)) {
        LogError << "failed to raise the target window" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
        release_window_locked();
        return false;
    }

    raised_ = true;

    // 目标窗口铺满屏幕时锚点在它下面，重新压回最上层，避免主指针身份被夺走
    if (HWND anchor = anchor_hwnd_.load()) {
        SetWindowPos(anchor, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
    }

    // 提升未必生效，注入前必须确认目标点真的落在目标窗口上，否则输入会打到遮挡它的那个窗口
    if (is_occluded(screen)) {
        LogError << "the target window is still occluded after being raised" << VAR(screen.x) << VAR(screen.y);
        release_window_locked();
        return false;
    }

    return true;
}

void AnchoredTouchInput::release_window_locked()
{
    if (!borrowed_ || !hwnd_) {
        return;
    }

    // 任何一项没能还原就保留 borrowed_，让后续的 touch_up 或 unprepare_window() 再试一次。
    // 提前清掉状态会让目标窗口永久停在 topmost 或最低不透明度上
    bool restored = true;

    if (raised_) {
        // 把 topmost 窗口插到非 topmost 窗口之后会剥掉它的 topmost 属性，
        // 因此只有前序兄弟与目标窗口的 topmost 状态一致时才用它还原
        HWND insert_after = original_topmost_ ? HWND_TOPMOST : HWND_NOTOPMOST;
        if (prev_sibling_ && IsWindow(prev_sibling_) && GetForegroundWindow() != hwnd_) {
            // 用户在借用期间把窗口调到了前台，插回原来的兄弟之后会把它压到别的窗口下面。
            // 这种情况下只摘掉 topmost 属性，不再恢复具体位置
            bool sibling_topmost = (GetWindowLongPtrW(prev_sibling_, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
            if (sibling_topmost == original_topmost_) {
                insert_after = prev_sibling_;
            }
        }

        if (SetWindowPos(hwnd_, insert_after, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER)) {
            raised_ = false;
        }
        else {
            LogError << "failed to restore the target window z-order, it may stay above other windows" << VAR_VOIDP(hwnd_)
                     << VAR(GetLastError());
            restored = false;
        }
    }

    // 不透明度放在最后恢复：顺序反过来目标窗口会有一段完全可见且仍位于最上层的时间。
    // 分层属性与 WS_EX_TRANSPARENT 的还原共用一次归属核对，这两项写的是同一份窗口状态，
    // 各判各的会出现一项识破接管、另一项照旧写回的情况
    if (dimmed_ || transparent_suppressed_) {
        switch (check_borrow_mark()) {
        case WindowOwnership::Taken:
            LogWarn << "the target window has been reconfigured by another module, skip restoring its layered attributes"
                    << VAR_VOIDP(hwnd_);
            transparent_suppressed_ = false;
            dimmed_ = false;
            break;

        case WindowOwnership::Unknown:
            LogWarn << "cannot read the target window layered state, will retry" << VAR_VOIDP(hwnd_);
            restored = false;
            break;

        case WindowOwnership::Ours:
            if (!restore_transparent()) {
                restored = false;
            }
            if (!undim_window()) {
                restored = false;
            }
            break;
        }
    }

    if (restored) {
        borrowed_ = false;
        prev_sibling_ = nullptr;
    }
}

void AnchoredTouchInput::release_window()
{
    std::lock_guard lock(window_mutex_);

    release_window_locked();
}

void AnchoredTouchInput::release_window_if_idle()
{
    {
        std::lock_guard lock(mutex_);
        if (!contacts_.empty()) {
            return;
        }
    }

    release_window();
}

void AnchoredTouchInput::unprepare_window()
{
    std::lock_guard lock(window_mutex_);

    if (!window_prepared_ || !hwnd_) {
        return;
    }

    release_window_locked();

    if (borrowed_) {
        LogError << "the target window state could not be fully restored" << VAR_VOIDP(hwnd_) << VAR(raised_) << VAR(dimmed_)
                 << VAR(transparent_suppressed_);
    }

    if (layered_applied_) {
        SetLastError(0);
        LONG_PTR exstyle = GetWindowLongPtrW(hwnd_, GWL_EXSTYLE);
        if (exstyle == 0 && GetLastError() != 0) {
            LogError << "GetWindowLongPtrW failed" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
            return;
        }

        // 样式是我们挂上的，承载在它上面的不透明度却未必还是我们的：截图侧的伪最小化会直接复用
        // 这份已经挂好的样式，把不透明度压到 0 并加上 WS_EX_TRANSPARENT。此时摘掉样式，
        // 那份不透明度会随之失效，本该隐形的窗口整个显出来，对方也再无从还原。
        // 因此只在这份分层状态没有被别人占用时才清，否则留着，等下一次调用再试。
        // Win32ControlUnitMgr::inactive() 先停截图侧再停输入侧，任务结束时的那一次重试即可清干净。
        // 样式已经不在则不算被占用——截图侧的还原是整份写回扩展样式，会把我们加的这一位一并抹掉，
        // 此时下面的清除退化成空操作，照常销账即可
        COLORREF color_key = 0;
        BYTE alpha = 0;
        DWORD layered_flags = 0;
        bool in_use = (exstyle & WS_EX_LAYERED) != 0
                      && ((exstyle & WS_EX_TRANSPARENT) != 0 || !GetLayeredWindowAttributes(hwnd_, &color_key, &alpha, &layered_flags)
                          || alpha != 255 || layered_flags != LWA_ALPHA);

        if (in_use) {
            LogWarn << "the layered state is in use by another module, keep WS_EX_LAYERED" << VAR_VOIDP(hwnd_) << VAR(alpha)
                    << VAR(layered_flags);
            return;
        }

        // 只清掉我们加的分层位，保留这期间目标程序自己改动的其他样式。
        // 原始扩展样式里没有该位，所以不存在误清的风险
        SetLastError(0);
        if (SetWindowLongPtrW(hwnd_, GWL_EXSTYLE, exstyle & ~static_cast<LONG_PTR>(WS_EX_LAYERED)) == 0 && GetLastError() != 0) {
            LogError << "failed to restore the target window ex-style" << VAR_VOIDP(hwnd_) << VAR(GetLastError());
            return;
        }

        layered_applied_ = false;
    }

    // 借用状态或分层样式只要还有一项没还清，就保留 window_prepared_，
    // 否则本函数下次会在开头直接返回，残留的改动再没有清理的机会
    if (borrowed_ || layered_applied_) {
        return;
    }

    window_prepared_ = false;
    window_prepare_ok_ = false;
}

LRESULT CALLBACK AnchoredTouchInput::anchor_wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param)
{
    switch (msg) {
    case WM_NCHITTEST:
        return HTCLIENT;

    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;

    case WM_POINTERACTIVATE:
        return PA_NOACTIVATE;

    // 这些消息必须由窗口自己处理掉并返回 0。交给 DefWindowProc 的话，
    // 系统认为窗口不消费指针输入，会把主指针提升成鼠标事件，光标就被抢走了。
    // 同理，这里不能做任何可能阻塞的事，处理慢了系统一样会走默认路径
    case WM_POINTERDOWN:
    case WM_POINTERUPDATE:
    case WM_POINTERUP:
    case WM_POINTERENTER:
    case WM_POINTERLEAVE:
    case WM_POINTERCAPTURECHANGED:
    case WM_NCPOINTERDOWN:
    case WM_NCPOINTERUPDATE:
    case WM_NCPOINTERUP:
    case WM_TOUCHHITTESTING:
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, w_param, l_param);
    }
}

MAA_CTRL_UNIT_NS_END
