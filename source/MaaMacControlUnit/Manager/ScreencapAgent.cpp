#include "ScreencapAgent.h"

#include "Screencap/CGWindowListScreencap.h"
#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

ScreencapAgent::ScreencapAgent(MaaMacScreencapMethod method, uint32_t windowId)
    : window_id_(windowId)
{
    LogInfo << VAR(method) << VAR(windowId);

    switch (method) {
    case MaaMacScreencapMethod_CGWindowList:
        screencap_ = std::make_shared<CGWindowListScreencap>(window_id_);
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
