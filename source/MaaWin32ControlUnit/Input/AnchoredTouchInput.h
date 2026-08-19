#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <thread>

#include "MaaControlUnit/ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

// 合成触控注入。
//
// 通过 CreateSyntheticPointerDevice / InjectSyntheticPointerInput 向系统注入触控接触点，
// 目标窗口收到的是 WM_POINTER 系列消息，全程不移动系统光标、不改变前台窗口。
//
// 关键在于「主指针」：一组触控里的第一个接触点是主指针，Windows 只会把主指针提升为鼠标事件，
// 而且主指针一旦确定，要等所有接触点都抬起才会产生新的主指针。
// 因此这里先在屏幕角落放一个微型锚点窗口，让它按住第一个接触点吃掉主指针身份，
// 真正的操作点作为第二个接触点注入，目标窗口只会收到纯触控消息。
//
// 合成指针的命中判定只认桌面当前的 Z 序，目标点被其他窗口遮挡时注入不会到达目标窗口，
// 所以按下前会把目标窗口提到 topmost 并降到最低分层透明度，所有接触点抬起后还原，
// 见 ensure_hittable / release_window_locked。借用不成功时 touch_down 直接失败，绝不注入，
// 否则输入会落到遮挡目标的那个窗口上。
//
// 为省掉每次借用都要切换扩展样式引起的闪烁，WS_EX_LAYERED 一旦挂上就保留到 inactive()
// 或空闲退出，由 unprepare_window() 清除。该样式随时可能被目标程序自己重设掉，
// 因此每次借用前都要重新确认，缺了就补挂；补不上或压不低不透明度时一律不提升，
// 否则目标窗口会完全可见地弹到最前面。
// 目标窗口当前已是分层窗口时不改样式，只借用它的不透明度，并在还原时写回原值；
// 这些原值按次读取，不跨借用沿用。若其分层属性由 UpdateLayeredWindow 设置，
// 碰它会破坏绘制，此时拒绝借用。
// 清除样式前还要确认这份分层状态没有被别人占用，见 unprepare_window()。
//
// 目标窗口的状态并非本方式独占：截图侧的伪最小化也会改写同一个窗口的扩展样式与不透明度，
// 因此归还前会核对借用痕迹是否还在，见 check_borrow_mark()。
class AnchoredTouchInput : public InputBase
{
public:
    explicit AnchoredTouchInput(HWND hwnd);

    virtual ~AnchoredTouchInput() override;

public: // from InputBase
    virtual MaaControllerFeature get_features() const override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

    virtual bool scroll(int dx, int dy) override;

    virtual void inactive() override;

private:
    struct ContactState
    {
        POINT screen_pos = {};
        bool pending_down = false;
        bool pending_up = false;
        bool active = false;
    };

    // 归还窗口前对借用痕迹的核对结果。目标程序或其他模块（例如截图侧的伪最小化还原）
    // 可能在借用期间重设目标窗口，此时写回借用前的旧值只会破坏对方刚设好的状态
    enum class WindowOwnership
    {
        Ours,    // 状态仍与借用时写入的一致，可以照常还原
        Taken,   // 已被外部改动，放弃还原
        Unknown, // 读不到真实状态，保留待还原项以便重试
    };

    static bool load_injection_api();

    static LRESULT CALLBACK anchor_wnd_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param);

    bool ensure_worker();
    void stop_worker();

    void worker_main();
    bool worker_setup();
    void worker_teardown();
    void worker_tick();
    void follow_target_window();

    bool wait_for_frames(std::unique_lock<std::mutex>& lock, int frames);
    bool wait_for_contact_active(std::unique_lock<std::mutex>& lock, int contact);
    bool wait_for_contact_released(std::unique_lock<std::mutex>& lock, int contact);

    bool to_screen(int x, int y, POINT& out) const;
    POINT compute_anchor_origin() const;

    bool is_occluded(POINT screen) const;
    bool anchor_covers(POINT screen) const;

    // 以下八个需要在持有 window_mutex_ 的情况下调用
    bool prepare_window();
    void begin_borrow();
    WindowOwnership check_borrow_mark() const;
    bool dim_window();
    bool undim_window();
    bool suppress_transparent();
    bool restore_transparent();
    void release_window_locked();

    bool ensure_restored();
    bool ensure_hittable(POINT screen);
    void release_window();
    void release_window_if_idle();
    void unprepare_window();

private:
    HWND hwnd_ = nullptr;

    std::thread worker_;
    std::atomic_bool running_ = false;
    std::atomic_bool ready_ = false;
    std::atomic_bool setup_failed_ = false;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::map<int, ContactState> contacts_;
    uint64_t frame_count_ = 0;
    std::chrono::steady_clock::time_point last_activity_ = std::chrono::steady_clock::now();

    // 注入线程创建与销毁，调用线程在提升目标窗口时需要一并把它压回最上层
    std::atomic<HWND> anchor_hwnd_ = nullptr;

    // 以下仅注入线程访问
    void* device_ = nullptr;
    POINT anchor_pos_ = {};
    bool anchor_down_ = false;
    RECT last_target_rect_ = {};
    std::wstring class_name_;

    // 对目标窗口样式与 Z 序的临时改动，由调用线程在 touch_down / touch_up 中操作
    std::mutex window_mutex_;
    bool window_prepared_ = false;
    bool window_prepare_ok_ = false;
    bool layered_applied_ = false;
    bool original_layered_ = false;
    bool original_topmost_ = false;
    COLORREF original_color_key_ = 0;
    BYTE original_alpha_ = 255;
    DWORD original_layered_flags_ = LWA_ALPHA;

    // borrowed_ 表示目标窗口正处于被临时改动的状态，下面四项记录具体改动了什么，
    // 一次触控序列结束后由 release_window_locked() 逐项还原
    bool borrowed_ = false;
    bool dimmed_ = false;
    bool raised_ = false;
    bool transparent_suppressed_ = false;
    bool restored_from_minimized_ = false;
    HWND prev_sibling_ = nullptr;

    // dim_window() 实际写入目标窗口的分层属性，供 check_borrow_mark() 核对
    COLORREF mark_color_key_ = 0;
    DWORD mark_layered_flags_ = LWA_ALPHA;
};

MAA_CTRL_UNIT_NS_END
