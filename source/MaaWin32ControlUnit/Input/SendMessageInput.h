#pragma once

#include "ControlUnit/ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "Utils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class SendMessageInput : public InputBase
{
public:
    SendMessageInput(HWND hwnd)
        : hwnd_(hwnd)
    {
    }

    virtual ~SendMessageInput() override = default;

public: // from InputBase
    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool is_touch_availabled() const override { return true; }

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool click_key(int key) override;

    virtual bool input_text(const std::string& text) override;

    virtual bool is_key_down_up_availabled() const override { return true; }

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

private:
    HWND hwnd_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
