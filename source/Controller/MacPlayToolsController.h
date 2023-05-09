#pragma once
#include "ControllerMgr.h"

MAA_CTRL_NS_BEGIN

struct MacPlayToolsControllerConfig
{
    static std::optional<MacPlayToolsControllerConfig> parse(const std::string& config_json);
};

class MacPlayToolsController : public ControllerMgr
{
public:
    MacPlayToolsController(const MacPlayToolsControllerConfig& config, MaaControllerCallback callback, void* callback_arg);
    virtual ~MacPlayToolsController() override;

    virtual MaaCtrlId click(int x, int y) override;
    virtual MaaCtrlId swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                            const std::vector<int>& step_delay) override;
    virtual MaaCtrlId screencap() override;

protected:
    // std::filesystem::path adb_path_;
    // std::string address_;
    MacPlayToolsControllerConfig playtools_config_;
};

MAA_CTRL_NS_END
