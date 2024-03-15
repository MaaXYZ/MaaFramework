#pragma once

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class AdbTapInput : public TouchInputBase
{
public:
    virtual ~AdbTapInput() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from TouchInputAPI
    virtual bool init(int swidth, int sheight, int orientation) override;

    virtual void deinit() override {}

    virtual bool set_wh(int swidth, int sheight, int orientation) override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

private:
    ProcessArgvGenerator click_argv_;
    ProcessArgvGenerator swipe_argv_;
};

class AdbKeyInput : public KeyInputBase
{
public:
    virtual ~AdbKeyInput() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from KeyInputAPI
    virtual bool init() override { return true; }

    virtual void deinit() override {}

    virtual bool press_key(int key) override;
    virtual bool input_text(const std::string& text) override;

private:
    ProcessArgvGenerator press_key_argv_;
    ProcessArgvGenerator input_text_argv_;
};

MAA_CTRL_UNIT_NS_END
