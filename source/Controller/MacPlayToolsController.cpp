#include "MacPlayToolsController.h"

MAA_CTRL_NS_BEGIN

std::optional<MacPlayToolsControllerConfig> MacPlayToolsControllerConfig::parse(const std::string& config_json)
{
    return std::optional<MacPlayToolsControllerConfig>();
}

MacPlayToolsController::MacPlayToolsController(const MacPlayToolsControllerConfig& config, MaaControllerCallback callback, void* callback_arg)
    : ControllerMgr(callback, callback_arg), playtools_config_(config)
{}

MacPlayToolsController::~MacPlayToolsController() {}

MaaCtrlId MacPlayToolsController::click(int x, int y)
{
    return MaaCtrlId();
}

MaaCtrlId MacPlayToolsController::swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                              const std::vector<int>& step_delay)
{
    return MaaCtrlId();
}

MaaCtrlId MacPlayToolsController::screencap()
{
    return MaaCtrlId();
}

MAA_CTRL_NS_END
