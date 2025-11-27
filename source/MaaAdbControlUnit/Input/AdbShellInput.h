#pragma once

#include "Base/UnitBase.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class AdbShellInput : public InputBase
{
public:
    virtual ~AdbShellInput() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from InputBase
    virtual bool init() override { return true; }

    virtual MaaControllerFeature get_features() const override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

    virtual bool scroll(int dx, int dy) override;

private:
    ProcessArgvGenerator click_argv_;
    ProcessArgvGenerator swipe_argv_;

    ProcessArgvGenerator click_key_argv_;
    ProcessArgvGenerator input_text_argv_;
};

MAA_CTRL_UNIT_NS_END
