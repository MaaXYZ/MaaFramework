#pragma once

#include "Base/UnitBase.h"
#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class NoneScreencap : public ScreencapBase
{
public:
    explicit NoneScreencap() = default;

    virtual ~NoneScreencap() override = default;

    std::optional<cv::Mat> screencap() override { return std::nullopt; }
};

MAA_CTRL_UNIT_NS_END
