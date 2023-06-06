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

void MinitouchController::_click(ClickParams param) {}

void MinitouchController::_swipe(SwipeParams param) {}

MAA_CTRL_NS_END
