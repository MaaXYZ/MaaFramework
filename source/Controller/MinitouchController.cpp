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

MaaCtrlId MinitouchController::click(int x, int y)
{
    return MaaCtrlId();
}

MaaCtrlId MinitouchController::swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                                     const std::vector<int>& step_delay)
{
    return MaaCtrlId();
}

MaaCtrlId MinitouchController::screencap()
{
    return MaaCtrlId();
}

MAA_CTRL_NS_END
