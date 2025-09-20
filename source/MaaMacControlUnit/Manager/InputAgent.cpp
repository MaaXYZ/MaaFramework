#include "InputAgent.h"

#include "Input/CGEventInput.h"
#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

InputAgent::InputAgent(MaaMacInputMethod method, uint32_t windowId)
    : window_id_(windowId)
{
    LogInfo << VAR(method) << VAR(windowId);

    switch (method) {
    case MaaMacInputMethod_CGEvent:
        input_ = std::make_shared<CGEventInput>(window_id_);
        break;
    default:
        LogError << "Unknown input method: " << static_cast<int>(method);
        break;
    }
}

bool InputAgent::click(int x, int y)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->click(x, y);
}

bool InputAgent::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->swipe(x1, y1, x2, y2, duration);
}

bool InputAgent::is_touch_availabled() const
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->is_touch_availabled();
}

bool InputAgent::touch_down(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->touch_down(contact, x, y, pressure);
}

bool InputAgent::touch_move(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->touch_move(contact, x, y, pressure);
}

bool InputAgent::touch_up(int contact)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->touch_up(contact);
}

bool InputAgent::click_key(int key)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->click_key(key);
}

bool InputAgent::input_text(const std::string& text)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->input_text(text);
}

bool InputAgent::is_key_down_up_availabled() const
{
    if (!input_) {
        LogError << "No available input method" << VAR(input_);
        return false;
    }

    return input_->is_key_down_up_availabled();
}

bool InputAgent::key_down(int key)
{
    if (!input_) {
        LogError << "No available input method" << VAR(input_);
        return false;
    }

    return input_->key_down(key);
}

bool InputAgent::key_up(int key)
{
    if (!input_) {
        LogError << "No available input method" << VAR(input_);
        return false;
    }

    return input_->key_up(key);
}

MAA_CTRL_UNIT_NS_END
