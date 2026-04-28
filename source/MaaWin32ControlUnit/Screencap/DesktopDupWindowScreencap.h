#pragma once

#include "Common/Conf.h"
#include "MaaUtils/SafeWindows.hpp"

#include "Base/UnitBase.h"
#include "DesktopDupScreencap.h"
#include "ScreenDCScreencap.h"

MAA_CTRL_UNIT_NS_BEGIN

class DesktopDupWindowScreencap : public DesktopDupScreencap
{
public:
    explicit DesktopDupWindowScreencap(HWND hwnd)
        : DesktopDupScreencap(hwnd)
        , screen_dc_fallback_(hwnd)
    {
    }

    virtual ~DesktopDupWindowScreencap() override = default;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override;
    virtual ScreencapInfo last_screencap_info() const override { return last_screencap_info_; }

private:
    std::optional<cv::Mat> screencap_from_screen_dc();
    RECT get_window_client_rect_screen() const;
    RECT get_output_desktop_coordinates() const;

    ScreenDCScreencap screen_dc_fallback_;
    ScreencapInfo last_screencap_info_ {};
};

MAA_CTRL_UNIT_NS_END
