#pragma once

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class CGWindowListScreencap : public ScreencapBase
{
public:
    explicit CGWindowListScreencap(uint32_t windowId)
        : window_id_(windowId)
    {
    }

    virtual ~CGWindowListScreencap() override = default;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override;

private:
    uint32_t window_id_ = 0;
};

MAA_CTRL_UNIT_NS_END
