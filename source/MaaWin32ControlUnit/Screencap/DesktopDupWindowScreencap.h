#pragma once

#include "Common/Conf.h"
#include "MaaUtils/SafeWindows.hpp"

#include "Base/UnitBase.h"
#include "DesktopDupScreencap.h"

MAA_CTRL_UNIT_NS_BEGIN

class DesktopDupWindowScreencap : public DesktopDupScreencap
{
public:
    explicit DesktopDupWindowScreencap(HWND hwnd)
        : DesktopDupScreencap(hwnd)
    {
    }

    virtual ~DesktopDupWindowScreencap() override = default;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override;

private:
    RECT get_window_client_rect_screen() const;
    RECT get_output_desktop_coordinates() const;
};

MAA_CTRL_UNIT_NS_END

