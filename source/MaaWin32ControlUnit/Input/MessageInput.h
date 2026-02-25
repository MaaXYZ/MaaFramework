#pragma once

#include "ControlUnit/ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "MaaUtils/SafeWindows.hpp"

#include <atomic>
#include <thread>

MAA_CTRL_UNIT_NS_BEGIN

class MessageInput : public InputBase
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

private:
    void send_activate();
    bool send_or_post_w(UINT message, WPARAM wParam, LPARAM lParam);

    // 准备鼠标位置：with_cursor_pos_ 模式下移动真实光标，with_window_pos_ 模式下移动窗口，返回 lParam
    LPARAM prepare_mouse_position(int x, int y);

    // WithWindowPos 模式：移动窗口使客户区坐标 (x,y) 与当前鼠标位置重合
    bool move_window_to_align_cursor(int x, int y);

    // helpers for cursor/window position
    POINT client_to_screen(int x, int y);
    void save_cursor_pos();
    void restore_cursor_pos();
    void save_window_pos();
    void restore_window_pos();

    // 保存/恢复当前模式对应的位置
    void save_pos();
    void restore_pos();

    void check_and_block_input();
    void unblock_input();

    // 获取 last_pos_，若未设置则返回窗口客户区中心坐标
    std::pair<int, int> get_target_pos() const;

    // WithWindowPos background tracking
    void tracking_thread_func();
    void process_pending_mouse_frame();
    std::thread tracking_thread_;
    std::atomic_bool tracking_exit_{ false };
    std::atomic_bool tracking_active_{ false };
    std::atomic_int tracking_x_{ 0 };
    std::atomic_int tracking_y_{ 0 };

    // 钩子存储的待处理鼠标位置（由 60fps 批处理消费）
    std::atomic_int pending_mouse_x_{ 0 };
    std::atomic_int pending_mouse_y_{ 0 };
    std::atomic_bool has_pending_mouse_{ false };

    // 目标进程挂起/恢复
    void open_target_process();
    void close_target_process();
    void suspend_target_process();
    void resume_target_process();
    HANDLE target_process_handle_{ nullptr };
    
    inline static std::atomic_bool hook_block_mouse_{ false };
    inline static std::atomic<MessageInput*> s_active_instance_{ nullptr };
    static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);

    const HWND hwnd_ = nullptr;
    const Config config_;

    std::pair<int, int> last_pos_;
    bool last_pos_set_ = false;

    POINT saved_cursor_pos_ = { 0, 0 };
    bool cursor_pos_saved_ = false;
    RECT saved_window_rect_ = { 0, 0, 0, 0 };
    bool window_pos_saved_ = false;
};

MAA_CTRL_UNIT_NS_END
