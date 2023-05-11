#include "PlayToolsController.h"

MAA_CTRL_NS_BEGIN

std::optional<PlayToolsControllerConfig> PlayToolsControllerConfig::parse(const std::string& config_json)
{
    return PlayToolsControllerConfig();
}

PlayToolsController::PlayToolsController(const PlayToolsControllerConfig& config, MaaControllerCallback callback,
                                         void* callback_arg)
    : ControllerMgr(callback, callback_arg), playtools_config_(config)
{}

PlayToolsController::~PlayToolsController() {}

bool PlayToolsController::do_connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    return false;
}

bool PlayToolsController::do_click(int x, int y)
{
    return false;
}

bool PlayToolsController::do_swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                             const std::vector<int>& step_delay)
{
    return false;
}

bool PlayToolsController::do_screencap(cv::Mat& mat)
{
    return false;
}

MAA_CTRL_NS_END
