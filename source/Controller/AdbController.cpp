#include "AdbController.h"

#include "Platform/PlatformFactory.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

#include <meojson/json.hpp>

MAA_CTRL_NS_BEGIN

void AdbController::ControlUnit::set_io(const std::shared_ptr<PlatformIO>& io_ptr)
{
    for (auto unit : units) {
        unit->set_io(io_ptr);
    }
}

bool AdbController::ControlUnit::parse(const json::value& config)
{
    bool ret = true;
    for (auto unit : units) {
        ret = ret && unit->parse(config);
    }
    return ret;
}

std::optional<AdbController::ControlUnit> AdbController::parse_config(
    const json::value& config, std::shared_ptr<MAA_CTRL_UNIT_NS::TouchInputBase> touch,
    std::shared_ptr<MAA_CTRL_UNIT_NS::KeyInputBase> key, std::shared_ptr<MAA_CTRL_UNIT_NS::ScreencapBase> screencap)
{
    {
        ControlUnit result;

        result.connection = std::make_shared<MAA_CTRL_UNIT_NS::Connection>();
        result.units.push_back(result.connection);
        result.device_info = std::make_shared<MAA_CTRL_UNIT_NS::DeviceInfo>();
        result.units.push_back(result.device_info);
        result.activity = std::make_shared<MAA_CTRL_UNIT_NS::Activity>();
        result.units.push_back(result.activity);

        auto touch_unit = std::dynamic_pointer_cast<MAA_CTRL_UNIT_NS::UnitBase>(touch);
        result.touch_input = std::move(touch);
        result.units.emplace_back(touch_unit); // 后面还要用
        auto key_unit = std::dynamic_pointer_cast<MAA_CTRL_UNIT_NS::UnitBase>(key);
        result.key_input = std::move(key);
        if (touch_unit != key_unit) {
            result.units.emplace_back(std::move(key_unit));
        }

        result.screencap = screencap;
        result.units.emplace_back(std::move(screencap));

        auto ret = result.parse(config);
        return ret ? std::make_optional(std::move(result)) : std::nullopt;
    }
}

AdbController::AdbController(std::string adb_path, std::string address, ControlUnit control_unit,
                             MaaControllerCallback callback, void* callback_arg)
    : ControllerMgr(callback, callback_arg), adb_path_(std::move(adb_path)), address_(std::move(address)),
      control_unit_(std::move(control_unit))
{
    platform_io_ = PlatformFactory::create();
    control_unit_.set_io(platform_io_);
}

AdbController::~AdbController() {}

std::string AdbController::get_uuid() const
{
    return control_unit_.device_info ? control_unit_.device_info->get_uuid() : std::string();
}

bool AdbController::_connect()
{
    std::map<std::string, std::string> replacement {
        { "{ADB}", adb_path_ },
        { "{ADB_SERIAL}", address_ },
    };
    control_unit_.connection->set_replacement(replacement);

    bool connected = control_unit_.connection->connect();
    if (!connected) {
        notifier.notify(MaaMsg_Controller_ConnectFalied, { { "why", "ConnectFailed" } });
        LogError << "failed to connect";
        return false;
    }

    for (auto unit : control_unit_.units) {
        unit->set_replacement(replacement);
    }

    if (!control_unit_.device_info->request_uuid()) {
        notifier.notify(MaaMsg_Controller_UUIDGetFailed);
        notifier.notify(MaaMsg_Controller_ConnectFalied, { { "why", "UUIDGetFailed" } });
        LogError << "failed to request_uuid";
        return false;
    }
    const auto& uuid = control_unit_.device_info->get_uuid();
    notifier.notify(MaaMsg_Controller_UUIDGot, { { "uuid", uuid } });

    if (!control_unit_.device_info->request_resolution()) {
        notifier.notify(MaaMsg_Controller_ResolutionGetFailed);
        notifier.notify(MaaMsg_Controller_ConnectFalied, { { "why", "ResolutionGetFailed" } });
        LogError << "failed to request_resolution";
        return false;
    }
    auto [w, h] = control_unit_.device_info->get_resolution();
    notifier.notify(MaaMsg_Controller_ResolutionGot, { { "resolution", { { "width", w }, { "height", h } } } });

    if (!control_unit_.screencap->init(w, h)) {
        notifier.notify(MaaMsg_Controller_ScreencapInitFailed);
        notifier.notify(MaaMsg_Controller_ConnectFalied, { { "why", "ScreencapInitFailed" } });
        LogError << "falied to init screencap";
        return false;
    }
    notifier.notify(MaaMsg_Controller_ScreencapInited);

    if (!control_unit_.touch_input->init(w, h)) {
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
    if (!control_unit_.touch_input->click(param.x, param.y)) {
        LogError << "failed to click";
    }
}

void AdbController::_swipe(SwipeParams param)
{
    if (!control_unit_.touch_input->swipe(param.steps)) {
        LogError << "failed to swipe";
    }
}

cv::Mat AdbController::_screencap()
{
    auto ret = control_unit_.screencap->screencap();
    if (!ret) {
        return cv::Mat();
    }
    return std::move(ret.value());
}

MAA_CTRL_NS_END
