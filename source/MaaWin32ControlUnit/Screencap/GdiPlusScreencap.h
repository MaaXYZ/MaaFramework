#pragma once

#include "MaaUtils/SafeWindows.hpp"

#include "Base/UnitBase.h"
#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class GdiPlusScreencap : public ScreencapBase
{
public:
    explicit GdiPlusScreencap(HWND hwnd)
        : hwnd_(hwnd)
    {
    }

    virtual ~GdiPlusScreencap() override = default;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override { return {}; }

private:
    bool init();
    void uninit();

    HWND hwnd_ = nullptr;
    bool initialized_ = false;
};

MAA_CTRL_UNIT_NS_END

