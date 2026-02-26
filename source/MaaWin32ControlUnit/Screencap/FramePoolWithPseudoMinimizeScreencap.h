#pragma once

#include "FramePoolScreencap.h"
#include "PseudoMinimizeHelper.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class FramePoolWithPseudoMinimizeScreencap : public ScreencapBase
{
public:
    explicit FramePoolWithPseudoMinimizeScreencap(HWND hwnd)
        : inner_(hwnd)
        , helper_(hwnd)
    {
        helper_.start();
    }

    virtual ~FramePoolWithPseudoMinimizeScreencap() override = default;

public: // from ScreencapBase
    virtual std::optional<cv::Mat> screencap() override
    {
        helper_.ensure_not_minimized();
        return inner_.screencap();
    }

private:
    FramePoolScreencap inner_;
    PseudoMinimizeHelper helper_;
};

MAA_CTRL_UNIT_NS_END
