#pragma once

#include "SendMessageWithCursorPosInput.h"

MAA_CTRL_UNIT_NS_BEGIN

/**
 * @brief 在 SendMessageWithCursorPosInput 基础上增加 BlockInput
 *
 * 操作期间阻塞用户输入，防止干扰。
 * 注意：BlockInput 需要管理员权限。
 */
class SendMessageWithCursorPosAndBlockInput : public SendMessageWithCursorPosInput
{
public:
    SendMessageWithCursorPosAndBlockInput(HWND hwnd)
        : SendMessageWithCursorPosInput(hwnd)
    {
    }

    virtual ~SendMessageWithCursorPosAndBlockInput() override;

public: // from InputBase
    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;
    virtual bool scroll(int dx, int dy) override;
};

MAA_CTRL_UNIT_NS_END

