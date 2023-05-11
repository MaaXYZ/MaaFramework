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

bool AdbController::do_connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    return false;
}

bool AdbController::do_click(int x, int y)
{
    return false;
}

bool AdbController::do_swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                               const std::vector<int>& step_delay)
{
    return false;
}

bool AdbController::do_screencap(cv::Mat &mat)
{
    return false;
}

MAA_CTRL_NS_END
