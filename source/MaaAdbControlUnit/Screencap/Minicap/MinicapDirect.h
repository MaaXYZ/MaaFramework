#pragma once

#include "MinicapBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class MinicapDirect : public MinicapBase
{
public:
    using MinicapBase::MinicapBase;

    virtual ~MinicapDirect() override;

public: // from ScreencapAPI
    virtual bool init() override;
    virtual void deinit() override;

    virtual std::optional<cv::Mat> screencap() override;
};

MAA_CTRL_UNIT_NS_END
