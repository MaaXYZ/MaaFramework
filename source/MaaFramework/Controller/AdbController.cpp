#include "AdbController.h"

#include "MaaFramework/MaaMsg.h"
#include "Utils/Logger.h"
#include "Utils/StringMisc.hpp"

#include <meojson/json.hpp>

MAA_CTRL_NS_BEGIN

AdbController::AdbController(std::string adb_path, std::string address,
                             std::shared_ptr<MAA_ADB_CTRL_UNIT_NS::ControlUnitAPI> unit_mgr,
                             MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg)
    : ControllerMgr(callback, callback_arg), adb_path_(std::move(adb_path)), address_(std::move(address)),
      unit_mgr_(std::move(unit_mgr))
{
    LogDebug << VAR(adb_path_) << VAR(address_) << VAR(unit_mgr_) << VAR(unit_mgr_->activity_obj())
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

    json::value details = {
        { "adb", path_to_utf8_string(adb_path_) },
        { "address", address_ },
    };

    bool connected = unit_mgr_->connection_obj()->connect();
    bool is_remote = address_.find(':') != std::string::npos;
    if (!connected && is_remote) {
        notifier.notify(MaaMsg_Controller_ConnectFailed, details | json::object { { "why", "ConnectFailed" } });
        LogError << "failed to connect";
        return false;
    }

    if (!unit_mgr_->device_info_obj()->request_uuid()) {
        notifier.notify(MaaMsg_Controller_UUIDGetFailed, details);
        notifier.notify(MaaMsg_Controller_ConnectFailed, details | json::object { { "why", "UUIDGetFailed" } });
        LogError << "failed to request_uuid";
        return false;
    }
    const auto& uuid = unit_mgr_->device_info_obj()->get_uuid();
    details |= { { "uuid", uuid } };

    notifier.notify(MaaMsg_Controller_UUIDGot, details | json::object { { "uuid", uuid } });

    if (!unit_mgr_->device_info_obj()->request_resolution()) {
        notifier.notify(MaaMsg_Controller_ResolutionGetFailed, details);
        notifier.notify(MaaMsg_Controller_ConnectFailed, details | json::object { { "why", "ResolutionGetFailed" } });
        LogError << "failed to request_resolution";
        return false;
    }
    auto [w, h] = unit_mgr_->device_info_obj()->get_resolution();
    resolution_ = { w, h };
    int orientation = unit_mgr_->device_info_obj()->request_orientation().value_or(0);
    details |= { { "resolution", { { "width", w }, { "height", h } } }, { "orientation", orientation } };

    notifier.notify(MaaMsg_Controller_ResolutionGot, details);

    if (!unit_mgr_->screencap_obj()->init(w, h)) {
        notifier.notify(MaaMsg_Controller_ScreencapInitFailed, details);
        notifier.notify(MaaMsg_Controller_ConnectFailed, details | json::object { { "why", "ScreencapInitFailed" } });
        LogError << "failed to init screencap";
        return false;
    }
    notifier.notify(MaaMsg_Controller_ScreencapInited, details);

    if (!unit_mgr_->touch_input_obj()->init(w, h, orientation)) {
        notifier.notify(MaaMsg_Controller_TouchInputInitFailed, details);
        notifier.notify(MaaMsg_Controller_ConnectFailed, details | json::object { { "why", "TouchInputInitFailed" } });
        LogError << "failed to init touch_input";
        return false;
    }
    notifier.notify(MaaMsg_Controller_TouchInputInited, details);

    notifier.notify(MaaMsg_Controller_ConnectSuccess, details);

    return true;
}

std::string AdbController::_get_uuid() const
{
    if (!unit_mgr_ || !unit_mgr_->device_info_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->device_info_obj());
        return {};
    }

    return unit_mgr_->device_info_obj()->get_uuid();
}

std::pair<int, int> AdbController::_get_resolution() const
{
    return resolution_;
}

bool AdbController::_click(ClickParam param)
{
    if (!unit_mgr_ || !unit_mgr_->touch_input_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->touch_input_obj());
        return false;
    }

    bool ret = unit_mgr_->touch_input_obj()->click(param.x, param.y);

    if (!ret) {
        LogError << "failed to click";
    }

    return ret;
}

bool AdbController::_swipe(SwipeParam param)
{
    if (!unit_mgr_ || !unit_mgr_->touch_input_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->touch_input_obj());
        return false;
    }

    bool ret = unit_mgr_->touch_input_obj()->swipe(param.x1, param.y1, param.x2, param.y2, param.duration);

    if (!ret) {
        LogError << "failed to swipe";
    }

    return ret;
}

bool AdbController::_touch_down(TouchParam param)
{
    if (!unit_mgr_ || !unit_mgr_->touch_input_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->touch_input_obj());
        return false;
    }

    bool ret = unit_mgr_->touch_input_obj()->touch_down(param.contact, param.x, param.y, param.pressure);

    if (!ret) {
        LogError << "failed to touch_down";
    }

    return ret;
}

bool AdbController::_touch_move(TouchParam param)
{
    if (!unit_mgr_ || !unit_mgr_->touch_input_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->touch_input_obj());
        return false;
    }

    bool ret = unit_mgr_->touch_input_obj()->touch_move(param.contact, param.x, param.y, param.pressure);

    if (!ret) {
        LogError << "failed to touch_move";
    }

    return ret;
}

bool AdbController::_touch_up(TouchParam param)
{
    if (!unit_mgr_ || !unit_mgr_->touch_input_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->touch_input_obj());
        return false;
    }

    bool ret = unit_mgr_->touch_input_obj()->touch_up(param.contact);

    if (!ret) {
        LogError << "failed to touch_up";
    }

    return ret;
}

bool AdbController::_press_key(PressKeyParam param)
{
    if (!unit_mgr_ || !unit_mgr_->key_input_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->key_input_obj());
        return false;
    }

    bool ret = unit_mgr_->key_input_obj()->press_key(param.keycode);

    if (!ret) {
        LogError << "failed to press_key";
    }

    return ret;
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

    return unit_mgr_->activity_obj()->start(param.package) && reinit_resolution();
}

bool AdbController::_stop_app(AppParam param)
{
    if (!unit_mgr_ || !unit_mgr_->activity_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->activity_obj());
        return false;
    }

    return unit_mgr_->activity_obj()->stop(param.package) && reinit_resolution();
}

bool AdbController::reinit_resolution()
{
    LogFunc;

    if (!unit_mgr_ || !unit_mgr_->device_info_obj() || !unit_mgr_->screencap_obj() || !unit_mgr_->touch_input_obj()) {
        LogError << "unit is nullptr" << VAR(unit_mgr_) << VAR(unit_mgr_->device_info_obj())
                 << VAR(unit_mgr_->screencap_obj()) << VAR(unit_mgr_->touch_input_obj());
        return false;
    }

    json::value details = {
        { "adb", path_to_utf8_string(adb_path_) },
        { "address", address_ },
        { "uuid", unit_mgr_->device_info_obj()->get_uuid() },
    };

    if (!unit_mgr_->device_info_obj()->request_resolution()) {
        notifier.notify(MaaMsg_Controller_ResolutionGetFailed, details);
        LogError << "failed to request_resolution";
        return false;
    }
    auto [w, h] = unit_mgr_->device_info_obj()->get_resolution();
    resolution_ = { w, h };
    int orientation = unit_mgr_->device_info_obj()->request_orientation().value_or(0);
    details |= { { "resolution", { { "width", w }, { "height", h } } }, { "orientation", orientation } };

    notifier.notify(MaaMsg_Controller_ResolutionGot, details);

    unit_mgr_->screencap_obj()->set_wh(w, h);
    unit_mgr_->touch_input_obj()->set_wh(w, h, orientation);

    return true;
}

MAA_CTRL_NS_END
