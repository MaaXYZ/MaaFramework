#include "InputAgent.h"

#include "Input/SeizeInput.h"
#include "Input/SendMessageInput.h"
#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

InputAgent::InputAgent(MaaWin32InputMethod method, HWND hWnd)
    : hwnd_(hWnd)
{
    LogInfo << VAR(method) << VAR(hWnd);

    switch (method) {
    case MaaWin32InputMethod_Seize:
        input_ = std::make_shared<SeizeInput>(hwnd_);
        break;
    case MaaWin32InputMethod_SendMessage:
        input_ = std::make_shared<SendMessageInput>(hwnd_);
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

bool InputAgent::press_key(int key)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->press_key(key);
}

bool InputAgent::input_text(const std::string& text)
{
    if (!input_) {
        LogError << "input_ is nullptr";
        return false;
    }

    return input_->input_text(text);
}

MAA_CTRL_UNIT_NS_END
