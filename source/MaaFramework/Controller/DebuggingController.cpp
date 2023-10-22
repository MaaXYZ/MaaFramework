#include "DebuggingController.h"

#include "MaaFramework/MaaMsg.h"
#include "Utils/Logger.h"
#include "Utils/StringMisc.hpp"

#include <meojson/json.hpp>

MAA_CTRL_NS_BEGIN

DebuggingController::DebuggingController(std::string read_path, std::string write_path,
                                         std::shared_ptr<MAA_DBG_CTRL_UNIT_NS::ControlUnitAPI> unit_mgr,
                                         MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg)
    : ControllerMgr(callback, callback_arg), read_path_(std::move(read_path)), write_path_(std::move(write_path)),
      unit_mgr_(std::move(unit_mgr))
{
    LogDebug << VAR(read_path_) << VAR(write_path_) << VAR(unit_mgr_) << VAR(unit_mgr_->device_info_obj())
             << VAR(unit_mgr_->screencap_obj());
}

DebuggingController::~DebuggingController() {}

std::string DebuggingController::get_uuid() const
{
    return read_path_;
}

bool DebuggingController::_connect()
{
    if (!unit_mgr_ || !unit_mgr_->device_info_obj() || !unit_mgr_->screencap_obj()
        /*|| !unit_mgr_->touch_input_obj() || !unit_mgr_->key_input_obj()*/) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->device_info_obj())
                 << VAR(unit_mgr_->screencap_obj());
        return false;
    }

    auto [screen_width, screen_height] = unit_mgr_->device_info_obj()->get_resolution();
    unit_mgr_->screencap_obj()->init(screen_width, screen_height);

    return true;
}

std::pair<int, int> DebuggingController::_get_resolution() const
{
    auto [screen_width, screen_height] = unit_mgr_->device_info_obj()->get_resolution();
    return { screen_width, screen_height };
}

bool DebuggingController::_click(ClickParam param)
{
    std::ignore = param;
    return true;
}

bool DebuggingController::_swipe(SwipeParam param)
{
    std::ignore = param;
    return true;
}

bool DebuggingController::_touch_down(TouchParam param)
{
    std::ignore = param;
    return true;
}

bool DebuggingController::_touch_move(TouchParam param)
{
    std::ignore = param;
    return true;
}

bool DebuggingController::_touch_up(TouchParam param)
{
    std::ignore = param;
    return true;
}

bool DebuggingController::_press_key(PressKeyParam param)
{
    std::ignore = param;
    return true;
}

cv::Mat DebuggingController::_screencap()
{
    if (!unit_mgr_ || !unit_mgr_->screencap_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->screencap_obj());
        return {};
    }

    auto ret = unit_mgr_->screencap_obj()->screencap();
    if (!ret) {
        return cv::Mat();
    }
    return std::move(ret.value());
}

bool DebuggingController::_start_app(AppParam param)
{
    std::ignore = param;
    return true;
}

bool DebuggingController::_stop_app(AppParam param)
{
    std::ignore = param;
    return true;
}

MAA_CTRL_NS_END
