#pragma once

#include "InputBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class TapTouchInput : public TouchInputBase, public UnitBase
{
public:
    bool parse(const json::value& config) override;

    bool init(int sw, int sh) override { return true; }

    bool click(int x, int y) override;
    bool swipe(const std::vector<Step>& steps) override;

private:
    Argv click_argv_;
    Argv swipe_argv_;
};

class TapKeyInput : public KeyInputBase, public UnitBase
{
public:
    bool parse(const json::value& config) override;

    bool press_key(int key) override;

private:
    Argv press_key_argv_;
};

MAA_CTRL_UNIT_NS_END
