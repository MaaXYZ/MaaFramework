#pragma once

#include "Base/UnitBase.h"
#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class ScreenDCScreencap : public ScreencapBase
{
public:
    explicit ScreenDCScreencap(HWND hwnd)
        : hwnd_(hwnd)
    {
    }

    virtual ~ScreenDCScreencap() override = default;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override;
    virtual ScreencapInfo last_screencap_info() const override { return last_screencap_info_; }

private:
    HWND hwnd_ = nullptr;
    ScreencapInfo last_screencap_info_ {};
};

MAA_CTRL_UNIT_NS_END
