#include "SendMessageInput.h"

MAA_CTRL_UNIT_NS_BEGIN

bool SendMessageInput::click(int x, int y)
{
    // TODO
    std::ignore = x;
    std::ignore = y;

    return false;
}

bool SendMessageInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    // TODO
    std::ignore = x1;
    std::ignore = y1;
    std::ignore = x2;
    std::ignore = y2;
    std::ignore = duration;

    return false;
}

bool SendMessageInput::touch_down(int contact, int x, int y, int pressure)
{
    // TODO
    std::ignore = contact;
    std::ignore = x;
    std::ignore = y;
    std::ignore = pressure;

    return false;
}

bool SendMessageInput::touch_move(int contact, int x, int y, int pressure)
{
    // TODO
    std::ignore = contact;
    std::ignore = x;
    std::ignore = y;
    std::ignore = pressure;

    return false;
}

bool SendMessageInput::touch_up(int contact)
{
    // TODO
    std::ignore = contact;

    return false;
}

bool SendMessageInput::press_key(int key)
{
    // TODO
    std::ignore = key;

    return false;
}

MAA_CTRL_UNIT_NS_END
