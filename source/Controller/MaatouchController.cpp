#include "MaatouchController.h"

MAA_CTRL_NS_BEGIN

std::optional<MaatouchConfig> MaatouchConfig::parse(const std::string& config_json)
{
    return std::optional<MaatouchConfig>();
}

MaatouchController::MaatouchController(const std::filesystem::path& adb_path, const std::string& address,
                                       const MaatouchConfig& config, MaaControllerCallback callback, void* callback_arg)
    : MinitouchController(adb_path, address, config, callback, callback_arg), maatouch_config_(config)
{}

MaatouchController::~MaatouchController() {}

MaaCtrlId MaatouchController::click(int x, int y)
{
    return MaaCtrlId();
}

MaaCtrlId MaatouchController::swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                                    const std::vector<int>& step_delay)
{
    return MaaCtrlId();
}

MaaCtrlId MaatouchController::screencap()
{
    return MaaCtrlId();
}

MAA_CTRL_NS_END
