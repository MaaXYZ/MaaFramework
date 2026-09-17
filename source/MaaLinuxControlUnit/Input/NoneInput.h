#pragma once

#include "Base/UnitBase.h"
#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class NoneInput : public InputBase
{
public:
    explicit NoneInput() = default;

    virtual ~NoneInput() override = default;

    MaaControllerFeature get_features() const override;
    bool click(int x, int y) override;
    bool swipe(int x1, int y1, int x2, int y2, int duration) override;
    bool touch_down(int contact, int x, int y, int pressure) override;
    bool touch_move(int contact, int x, int y, int pressure) override;
    bool touch_up(int contact) override;
    bool click_key(int key) override;
    bool input_text(const std::string& text) override;
    bool key_down(int key) override;
    bool key_up(int key) override;
    bool scroll(int dx, int dy) override;
};

MAA_CTRL_UNIT_NS_END
