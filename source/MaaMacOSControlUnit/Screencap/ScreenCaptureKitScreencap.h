#pragma once

#include "Base/UnitBase.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreenCaptureKitScreencap : public ScreencapBase
{
public:
    explicit ScreenCaptureKitScreencap(uint32_t window_id)
        : window_id_(window_id)
    {
    }

    virtual ~ScreenCaptureKitScreencap() override = default;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override;

private:
    uint32_t window_id_ = 0;
};

MAA_CTRL_UNIT_NS_END
