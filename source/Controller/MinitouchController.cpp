#include "MinitouchController.h"

MAA_CTRL_NS_BEGIN

MinitouchController::MinitouchController(const std::string& adb_path, const std::string& address,
                                         const json::value& config, MaaControllerCallback callback, void* callback_arg)
    : ControllerMgr(callback, callback_arg)
{}

MinitouchController::~MinitouchController() {}

bool MinitouchController::_connect()
{
    return false;
}

void MinitouchController::_click(ClickParams param) {}

void MinitouchController::_swipe(SwipeParams param) {}

cv::Mat MinitouchController::_screencap()
{
    return cv::Mat();
}

MAA_CTRL_NS_END
