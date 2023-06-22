#pragma once

#include "../UnitBase.h"

#include "../../ControllerMgr.h"

MAA_CTRL_UNIT_NS_BEGIN

class TouchInputBase
{
public:
    using Step = SwipeParams::Step;

    virtual bool init(int swidth, int sheight) = 0;

    virtual bool click(int x, int y) = 0;
    virtual bool swipe(const std::vector<Step>& steps) = 0;
};

class KeyInputBase
{
public:
    virtual bool press_key(int key) = 0;
};

MAA_CTRL_UNIT_NS_END
