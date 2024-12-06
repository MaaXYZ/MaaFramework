#pragma once

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class AdbShellInput : public InputBase
{
public:
    virtual ~AdbShellInput() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from InputBase
    virtual bool init() override { return true; }

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;
    virtual bool multi_swipe(const std::vector<SwipeParam>& swipes) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool press_key(int key) override;
    virtual bool input_text(const std::string& text) override;

private:
    ProcessArgvGenerator click_argv_;
    ProcessArgvGenerator swipe_argv_;

    ProcessArgvGenerator press_key_argv_;
    ProcessArgvGenerator input_text_argv_;
};

MAA_CTRL_UNIT_NS_END
