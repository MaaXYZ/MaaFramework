#include "ControlUnitMgr.h"

#include <meojson/json.hpp>

#include "MaaFramework/MaaMsg.h"
#include "Manager/InputAgent.h"
#include "Manager/ScreencapAgent.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

ControlUnitMgr::ControlUnitMgr(
    std::filesystem::path adb_path,
    std::string adb_serial,
    MaaAdbScreencapMethod screencap_methods,
    MaaAdbInputMethod input_methods,
    json::object config,
    std::filesystem::path agent_path)
    : adb_path_(std::move(adb_path))
    , adb_serial_(std::move(adb_serial))
    , config_(std::move(config))
    , screencap_methods_(screencap_methods)
    , input_methods_(input_methods)
    , agent_path_(std::move(agent_path))
    , unit_replacement_({ { "{ADB}", path_to_utf8_string(adb_path_) }, { "{ADB_SERIAL}", adb_serial_ } })
    , connection_(adb_path_, adb_serial_)
{
    device_list_.parse(config_);
    connection_.parse(config_);
    device_info_.parse(config_);
    activity_.parse(config_);

    device_list_.set_replacement(unit_replacement_);
    connection_.set_replacement(unit_replacement_);
    device_info_.set_replacement(unit_replacement_);
    activity_.set_replacement(unit_replacement_);
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

    if (screencap_methods_ != MaaAdbScreencapMethod_None) {
        screencap_ = std::make_shared<ScreencapAgent>(screencap_methods_, agent_path_);
        screencap_->parse(config_);
        screencap_->set_replacement(unit_replacement_);

        if (!screencap_->init()) {
            LogError << "failed to init screencap";
            return false;
        }

        register_observer(screencap_);
    }
    else {
        LogWarn << "screencap_methods_ is MaaAdbScreencapMethod_None";
    }

    if (input_methods_ != MaaAdbInputMethod_None) {
        input_ = std::make_shared<InputAgent>(input_methods_, agent_path_);
        input_->parse(config_);
        input_->set_replacement(unit_replacement_);

        if (!input_->init()) {
            LogError << "failed to init input";
            return false;
        }

        register_observer(input_);
    }
    else {
        LogWarn << "input_methods_ is MaaAdbInputMethod_None";
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
    bool ret = activity_.start_app(intent);
    if (ret) {
        on_app_started(intent);
    }
    return ret;
}

bool ControlUnitMgr::stop_app(const std::string& intent)
{
    bool ret = activity_.stop_app(intent);
    if (ret) {
        on_app_stopped(intent);
    }
    return ret;
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
        connection_.kill_server();
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

bool ControlUnitMgr::multi_swipe(const std::vector<SwipeParam>& swipes)
{
    if (!input_) {
        LogError << "input_ is null";
        return false;
    }

    return input_->multi_swipe(swipes);
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

    dispatch([&](const std::shared_ptr<ControlUnitSink>& sink) {
        if (!sink) {
            return;
        }
        sink->on_image_resolution_changed(pre, cur);
    });
}

void ControlUnitMgr::on_app_started(const std::string& intent)
{
    LogFunc;

    dispatch([&](const std::shared_ptr<ControlUnitSink>& sink) {
        if (!sink) {
            return;
        }
        sink->on_app_started(intent);
    });
}

void ControlUnitMgr::on_app_stopped(const std::string& intent)
{
    LogFunc;

    dispatch([&](const std::shared_ptr<ControlUnitSink>& sink) {
        if (!sink) {
            return;
        }
        sink->on_app_stopped(intent);
    });
}

MAA_CTRL_UNIT_NS_END
