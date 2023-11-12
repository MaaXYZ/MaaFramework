#pragma once

#include "ControlUnit/ControlUnitAPI.h"

#include <filesystem>

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class SendMessageToucher : public TouchInputBase
{
public:
    virtual ~SendMessageToucher() override = default;

public: // from TouchInputBase
    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;
};

MAA_CTRL_UNIT_NS_END
