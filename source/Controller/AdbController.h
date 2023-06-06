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

protected:
    virtual bool _connect() override;
    virtual void _click(ClickParams param) override;
    virtual void _swipe(SwipeParams param) override;
    virtual cv::Mat _screencap() override;

protected:
    std::filesystem::path adb_path_;
    std::string address_;
    AdbConfig adb_config_;
};

MAA_CTRL_NS_END
