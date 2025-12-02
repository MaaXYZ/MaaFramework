#pragma once

#include "ControlUnit/ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "MaaUtils/NonCopyable.hpp"
#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class MessageInput : public InputBase
{
public:
    enum class Mode
    {
        PostMessage,
        SendMessage
    };

    MessageInput(HWND hwnd, Mode mode, bool with_cursor_pos = false, bool block_input = false)
        : hwnd_(hwnd)
        , mode_(mode)
        , with_cursor_pos_(with_cursor_pos)
        , block_input_(block_input)
    {
    }

    virtual ~MessageInput() override = default;

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

protected:
    void ensure_foreground();
    bool send_or_post_w(UINT message, WPARAM wParam, LPARAM lParam);

    // helpers for with_cursor_pos
    POINT client_to_screen(int x, int y);
    void save_cursor_pos();
    void restore_cursor_pos();
    void set_cursor_to_client_pos(int x, int y);

    HWND hwnd_ = nullptr;
    Mode mode_;
    bool with_cursor_pos_;
    bool block_input_;

    std::pair<int, int> last_pos_;
    POINT saved_cursor_pos_ = { 0, 0 };
    bool cursor_pos_saved_ = false;

private:
    // RAII helper for BlockInput
    class BlockInputGuard : public NonCopyable
    {
    public:
        explicit BlockInputGuard(bool should_block)
            : should_block_(should_block)
        {
            if (should_block_) {
                BlockInput(TRUE);
            }
        }

        ~BlockInputGuard()
        {
            if (should_block_) {
                BlockInput(FALSE);
            }
        }

    private:
        const bool should_block_ = false;
    };

    // RAII helper for cursor position management (for scroll operations)
    class ScopedCursorPosition : public NonCopyable
    {
    public:
        ScopedCursorPosition(HWND hwnd, bool should_manage, int client_x, int client_y)
            : should_manage_(should_manage)
        {
            if (should_manage_) {
                // 保存当前光标位置
                GetCursorPos(&saved_pos_);

                // 移动光标到目标位置（客户区坐标转屏幕坐标）
                POINT screen_pos = { client_x, client_y };
                if (hwnd) {
                    ClientToScreen(hwnd, &screen_pos);
                }
                SetCursorPos(screen_pos.x, screen_pos.y);
            }
        }

        ~ScopedCursorPosition()
        {
            if (should_manage_) {
                // 恢复光标位置
                SetCursorPos(saved_pos_.x, saved_pos_.y);
            }
        }

    private:
        const bool should_manage_ = false;
        POINT saved_pos_ = { 0, 0 };
    };
};

MAA_CTRL_UNIT_NS_END
