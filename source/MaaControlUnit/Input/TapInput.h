#pragma once

#include "UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class TapTouchInput : public TouchInputBase
{
public:
    virtual ~TapTouchInput() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from TouchInputAPI
    virtual bool init(int swidth, int sheight, int orientation) override;
    virtual void deinit() override {}
    virtual void set_wh(int swidth, int sheight, int orientation) override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

private:
    Argv click_argv_;
    Argv swipe_argv_;
};

class TapKeyInput : public KeyInputBase
{
public:
    virtual ~TapKeyInput() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from KeyInputAPI
    virtual bool press_key(int key) override;

private:
    Argv press_key_argv_;
};

MAA_CTRL_UNIT_NS_END
