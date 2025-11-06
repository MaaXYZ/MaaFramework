#pragma once

#include "Common/Conf.h"
#include "MaaUtils/SafeWindows.hpp"

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class GdiPlusScreencap : public ScreencapBase
{
public:
    explicit GdiPlusScreencap(HWND hwnd)
        : hwnd_(hwnd)
    {
    }

    virtual ~GdiPlusScreencap() override;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override;

private:
    bool init();
    void uninit();

    HWND hwnd_ = nullptr;
    bool initialized_ = false;
};

MAA_CTRL_UNIT_NS_END

