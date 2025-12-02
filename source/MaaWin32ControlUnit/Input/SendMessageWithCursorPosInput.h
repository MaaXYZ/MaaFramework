#pragma once

#include "SendMessageInput.h"

MAA_CTRL_UNIT_NS_BEGIN

/**
 * @brief 结合 SendMessage 消息发送与实际光标移动的输入方式
 *
 * 继承自 SendMessageInput，在鼠标操作时额外使用 SetCursorPos 移动光标。
 * 专为原神等检测实际鼠标位置的游戏设计。
 */
class SendMessageWithCursorPosInput : public SendMessageInput
{
public:
    SendMessageWithCursorPosInput(HWND hwnd)
        : SendMessageInput(hwnd)
    {
    }

    virtual ~SendMessageWithCursorPosInput() override = default;

public: // from InputBase
    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;
    virtual bool scroll(int dx, int dy) override;

private:
    // 获取窗口客户区坐标对应的屏幕坐标
    POINT client_to_screen(int x, int y);

    // 保存当前光标位置
    void save_cursor_pos();

    // 恢复之前保存的光标位置
    void restore_cursor_pos();

    // 移动光标到指定位置（客户区坐标）
    void set_cursor_to_client_pos(int x, int y);

    POINT saved_cursor_pos_ = { 0, 0 };
    bool cursor_pos_saved_ = false;
};

MAA_CTRL_UNIT_NS_END
