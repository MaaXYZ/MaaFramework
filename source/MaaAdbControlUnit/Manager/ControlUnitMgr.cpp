#include "ControlUnitMgr.h"

#include <meojson/json.hpp>

#include "MaaFramework/MaaMsg.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

ControlUnitMgr::ControlUnitMgr(
    std::filesystem::path adb_path,
    std::string adb_serial,
    MaaControllerCallback callback,
    MaaCallbackTransparentArg callback_arg)
    : adb_path_(std::move(adb_path))
    , adb_serial_(std::move(adb_serial))
    , notifier(callback, callback_arg)
{
}

bool ControlUnitMgr::find_device(std::vector<std::string>& devices)
{
    auto opt = device_list_.request_devices();
    if (!opt) {
        LogError << "failed to find_device";
        return false;
    }

    devices = std::move(opt).value();
    return true;
}

bool ControlUnitMgr::connect()
{
    json::value details = {
        { "adb", path_to_utf8_string(adb_path_) },
        { "address", adb_serial_ },
    };

    bool connected = connection_.connect();
    bool is_remote = adb_serial_.find(':') != std::string::npos;
    if (!connected && is_remote) {
        notifier.notify(
            MaaMsg_Controller_ConnectFailed,
            details | json::object { { "why", "ConnectFailed" } });
        return false;
    }

    auto uuid_opt = device_info_.request_uuid();
    if (!uuid_opt) {
        notifier.notify(MaaMsg_Controller_UUIDGetFailed, details);
        notifier.notify(
            MaaMsg_Controller_ConnectFailed,
            details | json::object { { "why", "UUIDGetFailed" } });
        return false;
    }
    const auto& uuid = uuid_opt.value();
    details |= { { "uuid", uuid } };

    notifier.notify(MaaMsg_Controller_UUIDGot, details | json::object { { "uuid", uuid } });

    const auto& resolution_opt = device_info_.request_resolution();
    if (!resolution_opt) {
        notifier.notify(MaaMsg_Controller_ResolutionGetFailed, details);
        notifier.notify(
            MaaMsg_Controller_ConnectFailed,
            details | json::object { { "why", "ResolutionGetFailed" } });
        return false;
    }
    auto [width, height] = *resolution_opt;
    int orientation = device_info_.request_orientation().value_or(0);
    details |= { { "resolution", { { "width", width }, { "height", height } } },
                 { "orientation", orientation } };

    notifier.notify(MaaMsg_Controller_ResolutionGot, details);

    notifier.notify(MaaMsg_Controller_ConnectSuccess, details);

    if (screencap_) {
        if (!screencap_->init(width, height)) {
            LogError << "failed to init screencap";
            notifier.notify(MaaMsg_Controller_ScreencapInitFailed, details);
            return false;
        }
        notifier.notify(MaaMsg_Controller_ScreencapInited, details);
    }
    else {
        LogWarn << "screencap_ is null";
        notifier.notify(MaaMsg_Controller_ScreencapInitFailed, details);
    }

    if (touch_input_) {
        if (!touch_input_->init(width, height, orientation)) {
            LogError << "failed to init touch_input";
            notifier.notify(MaaMsg_Controller_TouchInputInitFailed, details);
            return false;
        }
        notifier.notify(MaaMsg_Controller_TouchInputInited, details);
    }
    else {
        LogWarn << "touch_input_ is null";
        notifier.notify(MaaMsg_Controller_TouchInputInitFailed, details);
    }

    if (key_input_) {
        if (!key_input_->init()) {
            LogError << "failed to init key_input";
            notifier.notify(MaaMsg_Controller_KeyInputInitFailed, details);
            return false;
        }
        notifier.notify(MaaMsg_Controller_KeyInputInited, details);
    }
    else {
        LogWarn << "key_input_ is null";
        notifier.notify(MaaMsg_Controller_KeyInputInitFailed, details);
    }

    return true;
}

bool ControlUnitMgr::request_uuid(std::string& uuid)
{
    auto opt = device_info_.request_uuid();
    if (!opt) {
        LogError << "failed to request_uuid";
        return false;
    }
    uuid = std::move(opt).value();
    return true;
}

bool ControlUnitMgr::request_resolution(int& width, int& height)
{
    auto opt = device_info_.request_resolution();
    if (!opt) {
        LogError << "failed to request_uuid";
        return false;
    }
    width = opt->first;
    height = opt->second;
    return true;
}

bool ControlUnitMgr::start_app(const std::string& intent)
{
    return activity_.start_app(intent);
}

bool ControlUnitMgr::stop_app(const std::string& intent)
{
    return activity_.stop_app(intent);
}

bool ControlUnitMgr::screencap(cv::Mat& image)
{
    constexpr int kMaxReconnectTimes = 3;
    constexpr int kMaxRescreencapTimes = 10;

    for (int i = 0; i < kMaxReconnectTimes; ++i) {
        for (int j = 0; j < kMaxRescreencapTimes; ++j) {
            if (_screencap(image)) {
                screencap_available_ = true;
                return true;
            }

            if (!screencap_available_) {
                // first time
                return false;
            }
            LogWarn << "re-screencap";
        }

        LogWarn << "re-connect";
        connect();
    }

    return false;
}

bool ControlUnitMgr::click(int x, int y)
{
    if (!touch_input_) {
        LogError << "touch_input_ is null";
        return false;
    }

    return touch_input_->click(x, y);
}

bool ControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!touch_input_) {
        LogError << "touch_input_ is null";
        return false;
    }

    return touch_input_->swipe(x1, y1, x2, y2, duration);
}

bool ControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    if (!touch_input_) {
        LogError << "touch_input_ is null";
        return false;
    }

    return touch_input_->touch_down(contact, x, y, pressure);
}

bool ControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    if (!touch_input_) {
        LogError << "touch_input_ is null";
        return false;
    }

    return touch_input_->touch_move(contact, x, y, pressure);
}

bool ControlUnitMgr::touch_up(int contact)
{
    if (!touch_input_) {
        LogError << "touch_input_ is null";
        return false;
    }

    return touch_input_->touch_up(contact);
}

bool ControlUnitMgr::press_key(int key)
{
    if (!key_input_) {
        LogError << "key_input_ is null";
        return false;
    }

    return key_input_->press_key(key);
}

bool ControlUnitMgr::input_text(const std::string& text)
{
    if (!key_input_) {
        LogError << "key_input_ is null";
        return false;
    }

    return key_input_->input_text(text);
}

bool ControlUnitMgr::parse(const json::value& config)
{
    bool ret = true;

    ret &= connection_.parse(config);
    ret &= device_info_.parse(config);
    ret &= activity_.parse(config);
    ret &= device_list_.parse(config);

    if (screencap_) {
        ret &= screencap_->parse(config);
    }

    if (touch_input_) {
        ret &= touch_input_->parse(config);
    }

    if (key_input_) {
        ret &= key_input_->parse(config);
    }

    return ret;
}

void ControlUnitMgr::set_replacement(const UnitBase::Replacement& replacement)
{
    device_list_.set_replacement(replacement);
    connection_.set_replacement(replacement);
    device_info_.set_replacement(replacement);
    activity_.set_replacement(replacement);

    if (touch_input_) {
        touch_input_->set_replacement(replacement);
    }
    if (key_input_) {
        key_input_->set_replacement(replacement);
    }
    if (screencap_) {
        screencap_->set_replacement(replacement);
    }
}

bool ControlUnitMgr::_screencap(cv::Mat& image)
{
    if (!screencap_) {
        LogError << "screencap_ is null";
        return false;
    }

    auto opt = screencap_->screencap();

    if (!opt) {
        LogError << "failed to screencap";
        return false;
    }

    image = std::move(opt).value();
    return true;
}

MAA_CTRL_UNIT_NS_END