#pragma once
#include "ControllerMgr.h"

#include <optional>

MAA_CTRL_NS_BEGIN

struct AdbConfig
{
    static std::optional<AdbConfig> parse(const std::string& config_json);
};

class AdbController : public ControllerMgr
{
public:
    AdbController(const std::filesystem::path& adb_path, const std::string& address, const AdbConfig& config,
                  MaaControllerCallback callback, void* callback_arg);
    virtual ~AdbController() override;

    virtual bool do_connect(const std::string& adb_path, const std::string& address, const std::string& config) override;
    virtual bool do_click(int x, int y) override;
    virtual bool do_swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                            const std::vector<int>& step_delay) override;
    virtual bool do_screencap(cv::Mat &mat) override;

protected:
    std::filesystem::path adb_path_;
    std::string address_;
    AdbConfig adb_config_;
};

MAA_CTRL_NS_END
