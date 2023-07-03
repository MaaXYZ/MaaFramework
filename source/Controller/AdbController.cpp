#include "AdbController.h"

#include "MaaMsg.h"
#include "MaaUtils/Logger.hpp"
#include "Utils/StringMisc.hpp"

#include <meojson/json.hpp>

MAA_CTRL_NS_BEGIN

AdbController::AdbController(std::string adb_path, std::string address,
                             std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> unit_mgr, MaaControllerCallback callback,
                             void* callback_arg)
    : ControllerMgr(callback, callback_arg), adb_path_(std::move(adb_path)), address_(std::move(address)),
      unit_mgr_(std::move(unit_mgr))
{}

AdbController::~AdbController()
{
    if (unit_mgr_) {
        unit_mgr_->screencap_obj()->deinit();
        unit_mgr_->touch_input_obj()->deinit();
    }
}

std::string AdbController::get_uuid() const
{
    return unit_mgr_->device_info_obj()->get_uuid();
}

bool AdbController::_connect()
{
    bool connected = unit_mgr_->connection_obj()->connect();
    if (!connected) {
        notifier.notify(MaaMsg_Controller_ConnectFalied, { { "why", "ConnectFailed" } });
        LogError << "failed to connect";
        return false;
    }

    if (!unit_mgr_->device_info_obj()->request_uuid()) {
        notifier.notify(MaaMsg_Controller_UUIDGetFailed);
        notifier.notify(MaaMsg_Controller_ConnectFalied, { { "why", "UUIDGetFailed" } });
        LogError << "failed to request_uuid";
        return false;
    }
    const auto& uuid = unit_mgr_->device_info_obj()->get_uuid();
    notifier.notify(MaaMsg_Controller_UUIDGot, { { "uuid", uuid } });

    if (!unit_mgr_->device_info_obj()->request_resolution()) {
        notifier.notify(MaaMsg_Controller_ResolutionGetFailed);
        notifier.notify(MaaMsg_Controller_ConnectFalied, { { "why", "ResolutionGetFailed" } });
        LogError << "failed to request_resolution";
        return false;
    }
    auto [w, h] = unit_mgr_->device_info_obj()->get_resolution();
    notifier.notify(MaaMsg_Controller_ResolutionGot, { { "resolution", { { "width", w }, { "height", h } } } });

    if (!unit_mgr_->screencap_obj()->init(w, h)) {
        notifier.notify(MaaMsg_Controller_ScreencapInitFailed);
        notifier.notify(MaaMsg_Controller_ConnectFalied, { { "why", "ScreencapInitFailed" } });
        LogError << "falied to init screencap";
        return false;
    }
    notifier.notify(MaaMsg_Controller_ScreencapInited);

    if (!unit_mgr_->touch_input_obj()->init(w, h)) {
        notifier.notify(MaaMsg_Controller_TouchInputInitFailed);
        notifier.notify(MaaMsg_Controller_ConnectFalied, { { "why", "TouchInputInitFailed" } });
        LogError << "falied to init touch_input";
        return false;
    }
    notifier.notify(MaaMsg_Controller_TouchInputInited);

    notifier.notify(MaaMsg_Controller_ConnectSuccess,
                    { { "uuid", uuid }, { "resolution", { { "width", w }, { "height", h } } } });

    return true;
}

void AdbController::_click(ClickParams param)
{
    if (!unit_mgr_->touch_input_obj()->click(param.x, param.y)) {
        LogError << "failed to click";
    }
}

void AdbController::_swipe(SwipeParams param)
{
    std::vector<MAA_CTRL_UNIT_NS::SwipeStep> steps;
    for (const auto& step : param.steps) {
        steps.emplace_back(MAA_CTRL_UNIT_NS::SwipeStep { step.x, step.y, step.delay });
    }

    if (!unit_mgr_->touch_input_obj()->swipe(steps)) {
        LogError << "failed to swipe";
    }
}

cv::Mat AdbController::_screencap()
{
    auto ret = unit_mgr_->screencap_obj()->screencap();
    if (!ret) {
        return cv::Mat();
    }
    return std::move(ret.value());
}

MAA_CTRL_NS_END
