#include "CustomController.h"

#include "Utils/Logger.hpp"

MAA_CTRL_NS_BEGIN

CustomController::CustomController(MaaCustomControllerHandle handle, MaaControllerCallback callback, void* callback_arg)
    : ControllerMgr(callback, callback_arg), handle_(handle)
{}

bool CustomController::_connect()
{
    return false;
}

void CustomController::_click(ClickParams param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->click) << VAR(param.x) << VAR(param.y);

    if (!handle_ || !handle_->click) {
        LogError << "handle_ or handle_->click is nullptr";
        return;
    }

    handle_->click(param.x, param.y);
}

void CustomController::_swipe(SwipeParams param) {}

cv::Mat CustomController::_screencap()
{
    return cv::Mat();
}

MAA_CTRL_NS_END
