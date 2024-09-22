#include "ControlUnitMgr.h"

#include <meojson/json.hpp>

#include "MaaFramework/MaaMsg.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

ControlUnitMgr::ControlUnitMgr(
    std::filesystem::path adb_path,
    std::string adb_serial,
    std::shared_ptr<ScreencapBase> screencap_unit,
    std::shared_ptr<InputBase> touch_unit)
    : adb_path_(std::move(adb_path))
    , adb_serial_(std::move(adb_serial))
    , connection_(adb_path_, adb_serial_)
    , input_(std::move(touch_unit))
    , screencap_(std::move(screencap_unit))
{
    register_observer(input_); // for on_image_resolution_changed

    set_replacement({
        { "{ADB}", path_to_utf8_string(adb_path_) },
        { "{ADB_SERIAL}", adb_serial_ },
    });
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
    if (!connection_.connect()) {
        LogError << "failed to connect" << VAR(adb_path_) << VAR(adb_serial_);
        return false;
    }

    if (screencap_) {
        if (!screencap_->init()) {
            LogError << "failed to init screencap";
            return false;
        }
    }
    else {
        LogWarn << "screencap_ is null";
    }

    if (input_) {
        if (!input_->init()) {
            LogError << "failed to init touch_input";
            return false;
        }
    }
    else {
        LogWarn << "input_ is null";
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
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->click(x, y);
}

bool ControlUnitMgr::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->swipe(x1, y1, x2, y2, duration);
}

bool ControlUnitMgr::touch_down(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->touch_down(contact, x, y, pressure);
}

bool ControlUnitMgr::touch_move(int contact, int x, int y, int pressure)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->touch_move(contact, x, y, pressure);
}

bool ControlUnitMgr::touch_up(int contact)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->touch_up(contact);
}

bool ControlUnitMgr::press_key(int key)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->press_key(key);
}

bool ControlUnitMgr::input_text(const std::string& text)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->input_text(text);
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

    if (input_) {
        ret &= input_->parse(config);
    }

    return ret;
}

void ControlUnitMgr::set_replacement(const UnitBase::Replacement& replacement)
{
    device_list_.set_replacement(replacement);
    connection_.set_replacement(replacement);
    device_info_.set_replacement(replacement);
    activity_.set_replacement(replacement);

    if (input_) {
        input_->set_replacement(replacement);
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

    auto& [width, height] = image_resolution_;

    if (width == 0 || height == 0) {
        width = image.cols;
        height = image.rows;
    }
    else if (width != image.cols || height != image.rows) {
        LogInfo << "Image size changed" << VAR(width) << VAR(height) << VAR(image.cols) << VAR(image.rows);
        auto pre = image_resolution_;

        width = image.cols;
        height = image.rows;

        on_image_resolution_changed(pre, image_resolution_);
    }

    return true;
}

void ControlUnitMgr::on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur)
{
    LogFunc;

    Dispatcher<ControlUnitSink>::dispatch([&](const std::shared_ptr<ControlUnitSink>& sink) {
        if (!sink) {
            return;
        }
        sink->on_image_resolution_changed(pre, cur);
    });
}

MAA_CTRL_UNIT_NS_END
