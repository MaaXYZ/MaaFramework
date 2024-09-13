#pragma once

#include "Base/UnitBase.h"
#include "MaaFramework/MaaDef.h"
#include "Utils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

class ScreencapAgent : public ScreencapBase
{
public:
    ScreencapAgent(MaaWin32ScreencapMethod method, HWND hWnd);
    virtual ~ScreencapAgent() override = default;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override;

private:
    HWND hwnd_ = nullptr;

    std::shared_ptr<ScreencapBase> screencap_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
