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

protected:
    virtual void _click(ClickParams param) override;
    virtual void _swipe(SwipeParams param) override;

protected:
    MaatouchConfig maatouch_config_;
};

MAA_CTRL_NS_END
