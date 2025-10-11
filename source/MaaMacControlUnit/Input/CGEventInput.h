#pragma once

#include "ControlUnit/ControlUnitAPI.h"

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class CGEventInput : public InputBase
{
public:
    CGEventInput(uint32_t windowId);

    virtual ~CGEventInput() override = default;

public: // from TouchBase
    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool is_touch_availabled() const override { return true; }

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;

    virtual bool is_key_down_up_availabled() const override { return false; }

    virtual bool key_down([[maybe_unused]] int key) override { return false; }

    virtual bool key_up([[maybe_unused]] int key) override { return false; }

private:
    uint32_t window_id_ = 0;
    pid_t pid_ = 0;

    double cache_x_ = 0;
    double cache_y_ = 0;
};

MAA_CTRL_UNIT_NS_END
