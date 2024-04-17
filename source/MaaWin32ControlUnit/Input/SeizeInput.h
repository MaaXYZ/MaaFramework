#pragma once

#include "ControlUnit/ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "Utils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

class SeizeInput
    : public TouchInputBase
    , public KeyInputBase
{
public:
    SeizeInput(HWND hwnd)
        : hwnd_(hwnd)
    {
    }

    virtual ~SeizeInput() override = default;

public: // from TouchInputBase
    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool is_micro_control_supported() { return true; }

public: // from KeyInputBase
    virtual bool press_key(int key) override;
    virtual bool input_text(const std::string& text) override;

private:
    HWND hwnd_ = nullptr;
    void ensure_foreground();
};

MAA_CTRL_UNIT_NS_END
