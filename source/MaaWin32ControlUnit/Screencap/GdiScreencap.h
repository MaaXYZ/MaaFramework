#pragma once

#include "Base/UnitBase.h"
#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

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
