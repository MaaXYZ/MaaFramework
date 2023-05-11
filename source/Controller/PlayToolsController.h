#pragma once
#include "ControllerMgr.h"

MAA_CTRL_NS_BEGIN

struct PlayToolsControllerConfig
{
    static std::optional<PlayToolsControllerConfig> parse(const std::string& config_json);
};

class PlayToolsController : public ControllerMgr
{
public:
    PlayToolsController(const PlayToolsControllerConfig& config, MaaControllerCallback callback, void* callback_arg);
    virtual ~PlayToolsController() override;

    virtual bool do_connect(const std::string& adb_path, const std::string& address,
                            const std::string& config) override;
    virtual bool do_click(int x, int y) override;
    virtual bool do_swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                          const std::vector<int>& step_delay) override;
    virtual bool do_screencap(cv::Mat& mat) override;

protected:
    // std::filesystem::path adb_path_;
    // std::string address_;
    PlayToolsControllerConfig playtools_config_;
};

MAA_CTRL_NS_END
