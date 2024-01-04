#pragma once

#include "ControlUnit/ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "Utils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

class Direct3dScreencap : public ScreencapBase
{
public:
    explicit Direct3dScreencap(HWND hwnd) : hwnd_(hwnd) {}
    virtual ~Direct3dScreencap() override = default;

public: // from TouchInputBase
    virtual std::optional<cv::Mat> screencap() override;

private:
    HWND hwnd_;
};

MAA_CTRL_UNIT_NS_END
