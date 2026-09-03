#include "NoneInput.h"

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN
MaaControllerFeature NoneInput::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool NoneInput::click(int x, int y)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_up instead"
             << VAR(x) << VAR(y);
    return false;
}

bool NoneInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_move/touch_up instead"
             << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool NoneInput::touch_down(int contact, int x, int y, int pressure)
{
    std::ignore = contact;
    std::ignore = x;
    std::ignore = y;
    std::ignore = pressure;

    return true;
}

bool NoneInput::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = contact;
    std::ignore = x;
    std::ignore = y;
    std::ignore = pressure;

    return true;
}

bool NoneInput::touch_up(int contact)
{
    std::ignore = contact;
    return true;
}

bool NoneInput::click_key(int key)
{
    std::ignore = key;
    return true;
}

bool NoneInput::input_text(const std::string& text)
{
    std::ignore = text;
    return true;
}

bool NoneInput::key_down(int key)
{
    std::ignore = key;
    return true;
}

bool NoneInput::key_up(int key)
{
    std::ignore = key;
    return true;
}

bool NoneInput::scroll(int dx, int dy)
{
    std::ignore = dx;
    std::ignore = dy;
    return true;
}

MAA_CTRL_UNIT_NS_END
