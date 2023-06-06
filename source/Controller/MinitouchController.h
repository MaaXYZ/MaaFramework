#pragma once
#include "AdbController.h"

MAA_CTRL_NS_BEGIN

struct MinitouchConfig : public AdbConfig
{
    static std::optional<MinitouchConfig> parse(const std::string& config_json);
};

class MinitouchController : public AdbController
{
public:
    MinitouchController(const std::filesystem::path& adb_path, const std::string& address,
                        const MinitouchConfig& config, MaaControllerCallback callback, void* callback_arg);
    virtual ~MinitouchController() override;

protected:
    virtual void _click(ClickParams param) override;
    virtual void _swipe(SwipeParams param) override;

protected:
    MinitouchConfig minitouch_config_;
};

MAA_CTRL_NS_END
