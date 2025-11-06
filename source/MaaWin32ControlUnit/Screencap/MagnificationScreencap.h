#pragma once

#include "Base/UnitBase.h"
#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class MagnificationScreencap : public ScreencapBase
{
public:
    explicit MagnificationScreencap(HWND hwnd)
        : hwnd_(hwnd)
    {
    }

    virtual ~MagnificationScreencap() override;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override;

private:
    bool init();
    void uninit();

    HWND hwnd_ = nullptr;
    HWND mag_window_ = nullptr;
    bool initialized_ = false;
};

MAA_CTRL_UNIT_NS_END

