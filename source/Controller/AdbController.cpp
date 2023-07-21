#include "AdbController.h"

#include "MaaMsg.h"
#include "MaaUtils/Logger.hpp"
#include "Utils/StringMisc.hpp"

#include <meojson/json.hpp>

MAA_CTRL_NS_BEGIN

AdbController::AdbController(std::string adb_path, std::string address,
                             std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> unit_mgr, MaaControllerCallback callback,
                             MaaCallbackTransparentArg callback_arg)
    : ControllerMgr(callback, callback_arg), adb_path_(std::move(adb_path)), address_(std::move(address)),
      unit_mgr_(std::move(unit_mgr))
{
    LogTrace << VAR(adb_path_) << VAR(address_) << VAR(unit_mgr_) << VAR(unit_mgr_->activity_obj())
             << VAR(unit_mgr_->connection_obj()) << VAR(unit_mgr_->device_info_obj()) << VAR(unit_mgr_->key_input_obj())
             << VAR(unit_mgr_->screencap_obj()) << VAR(unit_mgr_->touch_input_obj());
}

AdbController::~AdbController()
{
    if (!unit_mgr_) {
        return;
    }

    if (unit_mgr_->screencap_obj()) {
        unit_mgr_->screencap_obj()->deinit();
    }
    if (unit_mgr_->touch_input_obj()) {
        unit_mgr_->touch_input_obj()->deinit();
    }
}

std::string AdbController::get_uuid() const
{
    if (!unit_mgr_ || !unit_mgr_->device_info_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->device_info_obj());
        return {};
    }

    return unit_mgr_->device_info_obj()->get_uuid();
}

bool AdbController::_connect()
{
    if (!unit_mgr_ || !unit_mgr_->connection_obj() || !unit_mgr_->device_info_obj() || !unit_mgr_->screencap_obj() ||
        !unit_mgr_->touch_input_obj() || !unit_mgr_->key_input_obj() || !unit_mgr_->activity_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->connection_obj())
                 << VAR(unit_mgr_->device_info_obj()) << VAR(unit_mgr_->screencap_obj())
                 << VAR(unit_mgr_->touch_input_obj()) << VAR(unit_mgr_->key_input_obj())
                 << VAR(unit_mgr_->activity_obj());
        return false;
    }

    bool connected = unit_mgr_->connection_obj()->connect();
    if (!connected) {
        notifier.notify(MaaMsg_Controller_ConnectFailed, { { "why", "ConnectFailed" } });
        LogError << "failed to connect";
        return false;
    }

    if (!unit_mgr_->device_info_obj()->request_uuid()) {
        notifier.notify(MaaMsg_Controller_UUIDGetFailed);
        notifier.notify(MaaMsg_Controller_ConnectFailed, { { "why", "UUIDGetFailed" } });
        LogError << "failed to request_uuid";
        return false;
    }
    const auto& uuid = unit_mgr_->device_info_obj()->get_uuid();
    notifier.notify(MaaMsg_Controller_UUIDGot, { { "uuid", uuid } });

    if (!unit_mgr_->device_info_obj()->request_resolution()) {
        notifier.notify(MaaMsg_Controller_ResolutionGetFailed);
        notifier.notify(MaaMsg_Controller_ConnectFailed, { { "why", "ResolutionGetFailed" } });
        LogError << "failed to request_resolution";
        return false;
    }
    auto [w, h] = unit_mgr_->device_info_obj()->get_resolution();
    resolution_ = { w, h };
    notifier.notify(MaaMsg_Controller_ResolutionGot, { { "resolution", { { "width", w }, { "height", h } } } });

    if (!unit_mgr_->screencap_obj()->init(w, h)) {
        notifier.notify(MaaMsg_Controller_ScreencapInitFailed);
        notifier.notify(MaaMsg_Controller_ConnectFailed, { { "why", "ScreencapInitFailed" } });
        LogError << "failed to init screencap";
        return false;
    }
    notifier.notify(MaaMsg_Controller_ScreencapInited);

    if (!unit_mgr_->touch_input_obj()->init(w, h)) {
        notifier.notify(MaaMsg_Controller_TouchInputInitFailed);
        notifier.notify(MaaMsg_Controller_ConnectFailed, { { "why", "TouchInputInitFailed" } });
        LogError << "failed to init touch_input";
        return false;
    }
    notifier.notify(MaaMsg_Controller_TouchInputInited);

    notifier.notify(MaaMsg_Controller_ConnectSuccess,
                    { { "uuid", uuid }, { "resolution", { { "width", w }, { "height", h } } } });

    return true;
}

std::pair<int, int> AdbController::_get_resolution() const
{
    return resolution_;
}

void AdbController::_click(ClickParam param)
{
    if (!unit_mgr_ || !unit_mgr_->touch_input_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->touch_input_obj());
        return;
    }

    bool ret = unit_mgr_->touch_input_obj()->click(param.x, param.y);

    if (!ret) {
        LogError << "failed to click";
    }
}

void AdbController::_swipe(SwipeParam param)
{
    if (!unit_mgr_ || !unit_mgr_->touch_input_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->touch_input_obj());
        return;
    }

    std::vector<MAA_CTRL_UNIT_NS::SwipeStep> steps;
    for (const auto& step : param.steps) {
        steps.emplace_back(MAA_CTRL_UNIT_NS::SwipeStep { step.x, step.y, step.delay });
    }

    bool ret = unit_mgr_->touch_input_obj()->swipe(steps);

    if (!ret) {
        LogError << "failed to swipe";
    }
}

void AdbController::_press_key(PressKeyParam param)
{
    if (!unit_mgr_ || !unit_mgr_->key_input_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->key_input_obj());
        return;
    }

    bool ret = unit_mgr_->key_input_obj()->press_key(param.keycode);

    if (!ret) {
        LogError << "failed to press_key";
    }
}

cv::Mat AdbController::_screencap()
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

bool AdbController::_start_app(AppParam param)
{
    if (!unit_mgr_ || !unit_mgr_->activity_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->activity_obj());
        return false;
    }

    return unit_mgr_->activity_obj()->start(param.package);
}

bool AdbController::_stop_app(AppParam param)
{
    if (!unit_mgr_ || !unit_mgr_->activity_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->activity_obj());
        return false;
    }

    return unit_mgr_->activity_obj()->stop(param.package);
}

MAA_CTRL_NS_END
