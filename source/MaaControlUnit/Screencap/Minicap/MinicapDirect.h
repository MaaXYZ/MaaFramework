#pragma once

#include "MinicapBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class MinicapDirect : public MinicapBase
{
public:
    using MinicapBase::MinicapBase;

    virtual ~MinicapDirect() override = default;

public: // from ScreencapAPI
    virtual std::optional<cv::Mat> screencap() override;
};

MAA_CTRL_UNIT_NS_END
