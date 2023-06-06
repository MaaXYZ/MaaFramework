#include "AdbController.h"

MAA_CTRL_NS_BEGIN

std::optional<AdbConfig> AdbConfig::parse(const std::string& config_json)
{
    return AdbConfig();
}

AdbController::AdbController(const std::filesystem::path& adb_path, const std::string& address, const AdbConfig& config,
                             MaaControllerCallback callback, void* callback_arg)
    : ControllerMgr(callback, callback_arg), adb_path_(adb_path), address_(address), adb_config_(config)
{}

AdbController::~AdbController() {}

bool AdbController::_connect()
{
    return false;
}

void AdbController::_click(ClickParams param) {}

void AdbController::_swipe(SwipeParams param) {}

cv::Mat AdbController::_screencap()
{
    return cv::Mat();
}

MAA_CTRL_NS_END
