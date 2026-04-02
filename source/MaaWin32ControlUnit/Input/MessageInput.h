#pragma once

#include "MaaControlUnit/ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "MaaUtils/SafeWindows.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

MAA_CTRL_UNIT_NS_BEGIN

class MessageInput : public RelativeMoveInput
{
public:
    enum class Mode
    {
        SendMessage,
        PostMessage,
    };

    struct Config
    {
        Mode mode = Mode::SendMessage;
        bool with_cursor_pos = false;
        bool with_window_pos = false;
        bool block_input = false;
    };

    MessageInput(HWND hwnd, Config config);

    virtual ~MessageInput() override;

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

    virtual bool relative_move(int dx, int dy) override;

public: // mouse lock follow
    bool set_mouse_lock_follow(bool enabled);

private:
    using TrackingClock = std::chrono::steady_clock;
    using TrackingDeadlineTicks = TrackingClock::duration::rep;

    HWND send_activate();
    bool send_or_post_w(HWND target, UINT message, WPARAM wParam, LPARAM lParam);

    HWND get_active_hwnd();
    LPARAM make_mouse_lparam(HWND target, int x, int y);

    // 在发鼠标消息前把系统状态调整到目标窗口愿意接受的位置。
    LPARAM prepare_mouse_position(int x, int y);

    // WithWindowPos 模式：移动窗口使客户区坐标 (x,y) 与当前鼠标位置重合
    bool move_window_to_align_cursor(int x, int y);

    // helpers for cursor/window position
    POINT client_to_screen(int x, int y);
    void save_cursor_pos();
    void restore_cursor_pos();
    void save_window_pos();
    void restore_window_pos();
    void start_window_tracking(int x, int y);
    void request_stop_window_tracking();
    void maybe_stop_window_tracking();
    void stop_window_tracking();
    bool handle_hardware_mouse_move(const MSLLHOOKSTRUCT& mouse_info);

    // 统一封装不同输入模式的收尾，避免调用点分散处理光标/窗口状态。
    void save_pos();
    void finish_pos();
    void restore_pos();

    void check_and_block_input();
    void unblock_input();

    // 某些消息没有显式坐标时仍需要一个稳定锚点，否则滚轮等消息会落到不可预期位置。
    std::pair<int, int> get_target_pos() const;

    // WithWindowPos 通过后台追踪把真实鼠标位移折算回窗口位置，避免会话期间目标点漂移。
    void tracking_thread_func();
    void cleanup_tracking_thread(bool init_reported);
    void process_pending_mouse_frame();
    std::thread tracking_thread_;
    std::atomic_bool tracking_exit_ = false;
    std::atomic_bool tracking_active_ = false;
    std::atomic_int tracking_x_ = 0;
    std::atomic_int tracking_y_ = 0;
    std::atomic_uint64_t tracking_generation_ = 0;
    std::atomic_uint64_t tracking_stop_generation_ = 0;
    std::atomic<TrackingDeadlineTicks> tracking_stop_deadline_ticks_ = 0;

    // 钩子先累积硬件鼠标位移，再由 tracking 线程按固定帧率统一释放，避免每次移动都同步挪窗。
    std::atomic_int pending_mouse_x_ = 0;
    std::atomic_int pending_mouse_y_ = 0;
    std::atomic_bool has_pending_mouse_ = false;

    // 目标进程挂起/恢复
    void open_target_process();
    void close_target_process();
    void suspend_target_process();
    void resume_target_process();
    HANDLE target_process_handle_ { nullptr };

    inline static std::atomic_bool hook_block_mouse_ = false;
    inline static std::atomic<MessageInput*> s_active_instance_ = nullptr;
    static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    const HWND hwnd_ = nullptr;
    const Config config_;

    std::pair<int, int> last_pos_;
    bool last_pos_set_ = false;
    HWND gesture_target_ = nullptr;

    POINT saved_cursor_pos_ = { 0, 0 };
    bool cursor_pos_saved_ = false;
    RECT saved_window_rect_ = { 0, 0, 0, 0 };
    // 保留首次进入 WithWindowPos 会话前的窗口位置，避免一连串触摸操作反复覆盖原始锚点。
    bool window_pos_saved_ = false;

    bool activate_mouse_lock_follow();
    void deactivate_mouse_lock_follow();
    bool ensure_tracking_thread();
    bool ensure_rawinput_window();
    void process_mouse_lock_follow_frame();
    bool compute_window_center_on_cursor(const POINT& cursor, int& out_left, int& out_top);

    bool create_rawinput_window();
    void destroy_rawinput_window();
    void send_counter_move(int raw_dx, int raw_dy);
    bool handle_rawinput_message(LPARAM lParam);
    bool consume_synthetic_rawinput();
    static LRESULT CALLBACK RawInputWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    HWND rawinput_hwnd_ = nullptr;
    std::atomic_int counter_pending_ = 0;
    std::mutex tracking_state_mutex_;
    std::condition_variable tracking_state_cv_;
    bool tracking_thread_init_done_ = false;
    bool tracking_thread_init_ok_ = false;
    bool rawinput_ensure_requested_ = false;
    bool rawinput_ensure_done_ = false;
    bool rawinput_ensure_ok_ = false;

    std::atomic<bool> mouse_lock_follow_active_ = false;
    bool tracking_thread_started_for_lock_follow_ = false;

    POINT lock_anchor_cursor_ = { };
    RECT lock_anchor_window_ = { };
    int lock_offset_x_ = 0;
    int lock_offset_y_ = 0;
};

MAA_CTRL_UNIT_NS_END
