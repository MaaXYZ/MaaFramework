#include "SendMessageWithCursorPosAndBlockInput.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

SendMessageWithCursorPosAndBlockInput::~SendMessageWithCursorPosAndBlockInput()
{
    BlockInput(FALSE);
}

bool SendMessageWithCursorPosAndBlockInput::touch_down(int contact, int x, int y, int pressure)
{
    BlockInput(TRUE);
    return SendMessageWithCursorPosInput::touch_down(contact, x, y, pressure);
}

bool SendMessageWithCursorPosAndBlockInput::touch_up(int contact)
{
    bool ret = SendMessageWithCursorPosInput::touch_up(contact);
    BlockInput(FALSE);
    return ret;
}

bool SendMessageWithCursorPosAndBlockInput::scroll(int dx, int dy)
{
    BlockInput(TRUE);
    bool ret = SendMessageWithCursorPosInput::scroll(dx, dy);
    BlockInput(FALSE);
    return ret;
}

MAA_CTRL_UNIT_NS_END

