#include "AdbController.h"

#include "Platform/PlatformFactory.h"
#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

#include <meojson/json.hpp>

MAA_CTRL_NS_BEGIN

AdbController::AdbController(const std::string& adb_path, const std::string& address, const json::value& config,
                             MaaControllerCallback callback, void* callback_arg)
    : ControllerMgr(callback, callback_arg), adb_path_(adb_path), address_(address)
{
    platform_io_ = PlatformFactory::create();
    support_socket_ = platform_io_->support_socket_;

    if (!support_socket_) {
        LogError << "AdbController not support socket";
    }

    connection_unit_ = std::make_shared<MAA_CTRL_UNIT_NS::Connection>(platform_io_);
    if (!connection_unit_->parse(config)) {
        LogError << "AdbController connection_unit_ parse config failed";
        throw std::runtime_error("AdbController connection_unit_ parse config failed");
    }
}

AdbController::~AdbController() {}

bool AdbController::_connect()
{
    std::map<std::string, std::string> replacement {
        { "{ADB}", adb_path_ },
        { "{ADB_SERIAL}", address_ },
    };
    connection_unit_->set_replacement(replacement);

    return connection_unit_->connect();
}

void AdbController::_click(ClickParams param) {}

void AdbController::_swipe(SwipeParams param) {}

cv::Mat AdbController::_screencap()
{
    return cv::Mat();
}

MAA_CTRL_NS_END
