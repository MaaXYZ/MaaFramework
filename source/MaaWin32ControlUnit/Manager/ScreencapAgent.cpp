#include "ScreencapAgent.h"

#include "Screencap/DesktopDupScreencap.h"
#include "Screencap/FramePoolScreencap.h"
#include "Screencap/GdiScreencap.h"
#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

ScreencapAgent::ScreencapAgent(MaaWin32ScreencapMethod method, HWND hWnd)
    : hwnd_(hWnd)
{
    LogInfo << VAR(method) << VAR(hWnd);

    switch (method) {
    case MaaWin32ScreencapMethod_GDI:
        screencap_ = std::make_shared<GdiScreencap>(hwnd_);
        break;
    case MaaWin32ScreencapMethod_FramePool:
        screencap_ = std::make_shared<FramePoolScreencap>(hwnd_);
        break;
    case MaaWin32ScreencapMethod_DXGI_DesktopDup:
        screencap_ = std::make_shared<DesktopDupScreencap>();
        break;
    default:
        LogError << "Unknown screencap method: " << static_cast<int>(method);
        break;
    }
}

std::optional<cv::Mat> ScreencapAgent::screencap()
{
    if (!screencap_) {
        LogError << "screencap_ is nullptr";
        return std::nullopt;
    }

    return screencap_->screencap();
}

MAA_CTRL_UNIT_NS_END
