#pragma once

#include "ControlUnit/ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "Utils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

class GdiScreencap : public ScreencapBase
{
public:
    explicit GdiScreencap(HWND hwnd) : hwnd_(hwnd) {}
    virtual ~GdiScreencap() override = default;

public: // from TouchInputBase
    virtual std::optional<cv::Mat> screencap() override;

private:
    double window_screen_scale();

    HWND hwnd_;
};

MAA_CTRL_UNIT_NS_END
