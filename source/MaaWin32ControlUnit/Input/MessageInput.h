#pragma once

#include "ControlUnit/ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class MessageInput : public InputBase
{
public:
    enum class Mode
    {
        SendMessage,
        PostMessage,
    };

    MessageInput(HWND hwnd, Mode mode, bool with_cursor_pos = false, bool block_input = false)
        : hwnd_(hwnd)
        , mode_(mode)
        , with_cursor_pos_(with_cursor_pos)
        , block_input_(block_input)
    {
    }

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
    void ensure_foreground();
    bool send_or_post_w(UINT message, WPARAM wParam, LPARAM lParam);

    // 准备鼠标位置：with_cursor_pos_ 模式下移动真实光标，返回 lParam
    LPARAM prepare_mouse_position(int x, int y);

    // helpers for with_cursor_pos
    POINT client_to_screen(int x, int y);
    void save_cursor_pos();
    void restore_cursor_pos();

    // 获取 last_pos_，若未设置则返回窗口客户区中心坐标
    std::pair<int, int> get_target_pos() const;

    const HWND hwnd_ = nullptr;
    const Mode mode_ = Mode::SendMessage;
    const bool with_cursor_pos_ = false;
    const bool block_input_ = false;

    std::pair<int, int> last_pos_;
    bool last_pos_set_ = false;
    POINT saved_cursor_pos_ = { 0, 0 };
    bool cursor_pos_saved_ = false;
};

MAA_CTRL_UNIT_NS_END
