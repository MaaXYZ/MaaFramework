#pragma once

#include "MinicapBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class MAA_DEBUG_API MinicapDirect : public MinicapBase
{
public:
    virtual ~MinicapDirect() override = default;

    virtual std::optional<cv::Mat> screencap() override;
};

MAA_CTRL_UNIT_NS_END
