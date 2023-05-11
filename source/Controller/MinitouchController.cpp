#include "MinitouchController.h"

MAA_CTRL_NS_BEGIN

std::optional<MinitouchConfig> MinitouchConfig::parse(const std::string& config_json)
{
    return MinitouchConfig();
}

MinitouchController::MinitouchController(const std::filesystem::path& adb_path, const std::string& address,
                                         const MinitouchConfig& config, MaaControllerCallback callback,
                                         void* callback_arg)
    : AdbController(adb_path_, address, config, callback, callback_arg), minitouch_config_(config)
{}

MinitouchController::~MinitouchController() {}

bool MinitouchController::do_connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    return false;
}

bool MinitouchController::do_click(int x, int y)
{
    return false;
}

bool MinitouchController::do_swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                             const std::vector<int>& step_delay)
{
    return false;
}

bool MinitouchController::do_screencap(cv::Mat& mat)
{
    return false;
}

MAA_CTRL_NS_END
