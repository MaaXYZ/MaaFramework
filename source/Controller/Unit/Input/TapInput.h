#pragma once

#include "../UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class TapInput : public UnitBase
{
public:
    bool parse(const json::value& config);

    bool click(int x, int y);
    bool swipe(int x1, int y1, int x2, int y2, int duration);
    bool press_key(int key);

private:
    Argv click_argv_;
    Argv swipe_argv_;
    Argv press_key_argv_;
};

MAA_CTRL_UNIT_NS_END
