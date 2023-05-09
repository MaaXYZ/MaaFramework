#include "PlayToolsController.h"

MAA_CTRL_NS_BEGIN

std::optional<PlayToolsControllerConfig> PlayToolsControllerConfig::parse(const std::string& config_json)
{
    return std::optional<PlayToolsControllerConfig>();
}

PlayToolsController::PlayToolsController(const PlayToolsControllerConfig& config, MaaControllerCallback callback, void* callback_arg)
    : ControllerMgr(callback, callback_arg), playtools_config_(config)
{}

PlayToolsController::~PlayToolsController() {}

MaaCtrlId PlayToolsController::click(int x, int y)
{
    return MaaCtrlId();
}

MaaCtrlId PlayToolsController::swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                              const std::vector<int>& step_delay)
{
    return MaaCtrlId();
}

MaaCtrlId PlayToolsController::screencap()
{
    return MaaCtrlId();
}

MAA_CTRL_NS_END
