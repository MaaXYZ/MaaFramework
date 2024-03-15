#pragma once

#include "Base/UnitBase.h"
#include "Utils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

class GdiScreencap : public ScreencapBase
{
public:
    explicit GdiScreencap(HWND hwnd)
        : hwnd_(hwnd)
    {
    }

    virtual ~GdiScreencap() override = default;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override;

private:
    HWND hwnd_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
