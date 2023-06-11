#include "MaatouchController.h"

MAA_CTRL_NS_BEGIN

MaatouchController::MaatouchController(const std::string& adb_path, const std::string& address,
                                       const json::value& config, MaaControllerCallback callback, void* callback_arg)
    : ControllerMgr(callback, callback_arg)
{}

MaatouchController::~MaatouchController() {}

bool MaatouchController::_connect()
{
    return false;
}

void MaatouchController::_click(ClickParams param) {}

void MaatouchController::_swipe(SwipeParams param) {}

cv::Mat MaatouchController::_screencap()
{
    return cv::Mat();
}

MAA_CTRL_NS_END
