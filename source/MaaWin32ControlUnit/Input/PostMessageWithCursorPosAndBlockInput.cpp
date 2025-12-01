#include "PostMessageWithCursorPosAndBlockInput.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

PostMessageWithCursorPosAndBlockInput::~PostMessageWithCursorPosAndBlockInput()
{
    BlockInput(FALSE);
}

bool PostMessageWithCursorPosAndBlockInput::touch_down(int contact, int x, int y, int pressure)
{
    BlockInput(TRUE);
    return PostMessageWithCursorPosInput::touch_down(contact, x, y, pressure);
}

bool PostMessageWithCursorPosAndBlockInput::touch_up(int contact)
{
    bool ret = PostMessageWithCursorPosInput::touch_up(contact);
    BlockInput(FALSE);
    return ret;
}

bool PostMessageWithCursorPosAndBlockInput::scroll(int dx, int dy)
{
    BlockInput(TRUE);
    bool ret = PostMessageWithCursorPosInput::scroll(dx, dy);
    BlockInput(FALSE);
    return ret;
}

MAA_CTRL_UNIT_NS_END

