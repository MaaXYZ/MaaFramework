#pragma once

#include "MinicapBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class MinicapDirect
    : public MinicapBase
    , public DeviceInfoSink
{
public:
    using MinicapBase::MinicapBase;

    virtual ~MinicapDirect() override = default;

public: // from ScreencapAPI
    virtual bool init() override;

    virtual void deinit() override {}

public: // from DeviceInfoSink
    virtual void on_display_changed(int width, int height) override;

    virtual std::optional<cv::Mat> screencap() override;

private:
    int display_width_ = 0;
    int display_height_ = 0;
};

MAA_CTRL_UNIT_NS_END
