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

bool MaatouchController::do_connect(const std::string& adb_path, const std::string& address, const std::string& config)
{
    return false;
}

bool MaatouchController::do_click(int x, int y)
{
    return false;
}

bool MaatouchController::do_swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                             const std::vector<int>& step_delay)
{
    return false;
}

bool MaatouchController::do_screencap(cv::Mat& mat)
{
    return false;
}

MAA_CTRL_NS_END
