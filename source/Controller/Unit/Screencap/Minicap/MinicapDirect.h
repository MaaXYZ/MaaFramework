#pragma once

#include "MinicapBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class MAAAPI_DEBUG MinicapDirect : public MinicapBase
{
public:
    virtual ~MinicapDirect() override = default;

    virtual std::optional<cv::Mat> screencap() override;
};

MAA_CTRL_UNIT_NS_END
