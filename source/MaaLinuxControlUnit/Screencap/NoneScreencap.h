#pragma once

#include "Base/UnitBase.h"
#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class NoneScreencap : public ScreencapBase
{
public:
    explicit NoneScreencap() = default;

    virtual ~NoneScreencap() override = default;

    bool screencap(/*out*/ cv::Mat&) override { return false; }
};

MAA_CTRL_UNIT_NS_END
