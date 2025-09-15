#pragma once

#include "Base/UnitBase.h"
#include "MaaFramework/MaaDef.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapAgent : public ScreencapBase
{
public:
    ScreencapAgent(MaaMacScreencapMethod method, uint32_t windowId);
    virtual ~ScreencapAgent() override = default;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override;

private:
    uint32_t window_id_ = 0;

    std::shared_ptr<ScreencapBase> screencap_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
