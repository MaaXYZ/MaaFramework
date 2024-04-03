#pragma once

#include "ControlUnit/ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "Utils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

class SendMessageInput
    : public TouchInputBase
    , public KeyInputBase
{
public:
    SendMessageInput(HWND hwnd)
        : hwnd_(hwnd)
    {
    }

    virtual ~SendMessageInput() override = default;

public: // from TouchInputBase
    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

public: // from KeyInputBase
    virtual bool press_key(int key) override;
    virtual bool input_text(const std::string& text) override;

private:
    HWND hwnd_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
