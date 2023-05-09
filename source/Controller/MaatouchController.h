#pragma once
#include "MinitouchController.h"

MAA_CTRL_NS_BEGIN

struct MaatouchConfig : public MinitouchConfig
{
    static std::optional<MaatouchConfig> parse(const std::string& config_json);
};

class MaatouchController : public MinitouchController
{
public:
    MaatouchController(const std::filesystem::path& adb_path, const std::string& address, const MaatouchConfig& config,
                       MaaControllerCallback callback, void* callback_arg);
    virtual ~MaatouchController() override;

    virtual MaaCtrlId click(int x, int y) override;
    virtual MaaCtrlId swipe(const std::vector<int>& x_steps, const std::vector<int>& y_steps,
                            const std::vector<int>& step_delay) override;
    virtual MaaCtrlId screencap() override;

protected:
    MaatouchConfig maatouch_config_;
};

MAA_CTRL_NS_END
