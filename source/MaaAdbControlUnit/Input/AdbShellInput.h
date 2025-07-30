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

    virtual bool is_touch_availabled() const override { return false; }
    virtual bool is_multi_touch_availabled() const override { return false; }

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;
    
    virtual bool is_key_down_up_availabled() const override { return false; }

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

private:
    ProcessArgvGenerator click_argv_;
    ProcessArgvGenerator swipe_argv_;

    ProcessArgvGenerator click_key_argv_;
    ProcessArgvGenerator input_text_argv_;
};

MAA_CTRL_UNIT_NS_END
