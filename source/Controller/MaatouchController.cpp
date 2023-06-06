#include "MaatouchController.h"

MAA_CTRL_NS_BEGIN

std::optional<MaatouchConfig> MaatouchConfig::parse(const std::string& config_json)
{
    return MaatouchConfig();
}

MaatouchController::MaatouchController(const std::filesystem::path& adb_path, const std::string& address,
                                       const MaatouchConfig& config, MaaControllerCallback callback, void* callback_arg)
    : MinitouchController(adb_path, address, config, callback, callback_arg), maatouch_config_(config)
{}

MaatouchController::~MaatouchController() {}

void MaatouchController::_click(ClickParams param) {}

void MaatouchController::_swipe(SwipeParams param) {}

MAA_CTRL_NS_END
