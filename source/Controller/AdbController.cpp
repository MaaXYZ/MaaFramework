#include "AdbController.h"

#include "Platform/PlatformFactory.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

#include <meojson/json.hpp>

MAA_CTRL_NS_BEGIN

void AdbController::ControlUnit::set_io(const std::shared_ptr<PlatformIO>& io_ptr)
{
    connection.set_io(io_ptr);
    device_info.set_io(io_ptr);
    activity.set_io(io_ptr);
    tap_input.set_io(io_ptr);
    screencap.set_io(io_ptr);
}

bool AdbController::ControlUnit::parse(const json::value& config)
{
    return connection.parse(config) && device_info.parse(config) && activity.parse(config) && tap_input.parse(config) &&
           screencap.parse(config);
}

AdbController::AdbController(std::string adb_path, std::string address, ControlUnit control_unit,
                             MaaControllerCallback callback, void* callback_arg)
    : ControllerMgr(callback, callback_arg), adb_path_(std::move(adb_path)), address_(std::move(address)),
      control_unit_(std::move(control_unit))
{
    platform_io_ = PlatformFactory::create();
    control_unit_.set_io(platform_io_);
}

std::optional<AdbController::ControlUnit> AdbController::parse_config(const json::value& config)
{
    ControlUnit result;
    bool ret = result.parse(config);

    return ret ? std::make_optional(std::move(result)) : std::nullopt;
}

AdbController::~AdbController() {}

bool AdbController::_connect()
{
    std::map<std::string, std::string> replacement {
        { "{ADB}", adb_path_ },
        { "{ADB_SERIAL}", address_ },
    };
    control_unit_.connection.set_replacement(replacement);

    bool connected = control_unit_.connection.connect();
    if (!connected) {
        notifier.notify(MAAMSG_CONTROLLER_CONNECT_FAILED);
        LogError << "failed to connect";
        return false;
    }

    notifier.notify(MAAMSG_CONTROLLER_CONNECTED);

    control_unit_.device_info.set_replacement(replacement);
    if (!control_unit_.device_info.request_uuid()) {
        notifier.notify(MAAMSG_CONTROLLER_UUID_GET_FAILED);
        LogError << "failed to request_uuid";
        return false;
    }
    const auto& uuid = control_unit_.device_info.get_uuid();
    notifier.notify(MAAMSG_CONTROLLER_UUID_GOT, { { "uuid", uuid } });

    if (!control_unit_.device_info.request_resolution()) {
        notifier.notify(MAAMSG_CONTROLLER_RESOLUTION_GET_FAILED);
        LogError << "failed to request_resolution";
        return false;
    }
    auto [w, h] = control_unit_.device_info.get_resolution();
    notifier.notify(MAAMSG_CONTROLLER_RESOLUTION_GOT, { { "resolution", { { "width", w }, { "height", h } } } });

    control_unit_.screencap.set_replacement(replacement);
    if (!control_unit_.screencap.init(w, h)) {
        notifier.notify(MAAMSG_CONTROLLER_SCREENCAP_INIT_FAILED);
        LogError << "falied to init screencap";
        return false;
    }
    notifier.notify(MAAMSG_CONTROLLER_SCREENCAP_INITED);

    notifier.notify(MAAMSG_CONTROLLER_CONNECT_SUCCESS,
                    { { "uuid", uuid }, { "resolution", { { "width", w }, { "height", h } } } });

    return true;
}

void AdbController::_click(ClickParams param) {}

void AdbController::_swipe(SwipeParams param) {}

cv::Mat AdbController::_screencap()
{
    return cv::Mat();
}

MAA_CTRL_NS_END
