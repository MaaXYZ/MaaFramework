#pragma once

#include "Base/UnitBase.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class InputAgent : public InputBase
{
public:
    enum class Method
    {
        UnknownYet,
        AdbShell,
        MinitouchAndAdbKey,
        Maatouch,
        MuMuPlayerExtras,
    };

public:
    InputAgent(MaaAdbInputMethod methods, const std::filesystem::path& agent_path);
    virtual ~InputAgent() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from InputBase
    virtual bool init() override;

    virtual MaaControllerFeature get_features() const override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool click_key(int key) override;
    virtual bool input_text(const std::string& text) override;

    virtual bool key_down(int key) override;
    virtual bool key_up(int key) override;

    virtual bool scroll(int dx, int dy) override;

public: // from ControlUnitSink
    virtual void on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur) override;
    virtual void on_app_started(const std::string& intent) override;
    virtual void on_app_stopped(const std::string& intent) override;

private:
    std::vector<std::pair<Method, std::shared_ptr<InputBase>>> units_;
    std::shared_ptr<InputBase> active_unit_;
};

MAA_CTRL_UNIT_NS_END
