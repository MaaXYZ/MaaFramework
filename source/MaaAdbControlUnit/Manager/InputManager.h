#pragma once

#include <vector>

#include "Base/UnitBase.h"

MAA_CTRL_UNIT_NS_BEGIN

class InputManager : public InputBase
{
public:
    enum class Method
    {
        UnknownYet,
        AdbShell,
        MinitouchAndAdbKey,
        Maatouch,
        MumuExternalRendererIpc,
    };
    using MethodVec = std::vector<Method>;

public:
    InputManager(const MethodVec& input_methods, const std::filesystem::path& agent_path);
    virtual ~InputManager() override = default;

public: // from UnitBase
    virtual bool parse(const json::value& config) override;

public: // from InputBase
    virtual bool init() override;
    virtual void deinit() override;

    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool press_key(int key) override;
    virtual bool input_text(const std::string& text) override;

private:
    std::vector<std::pair<Method, std::shared_ptr<InputBase>>> units_;
    std::shared_ptr<InputBase> active_unit_;
};

std::ostream& operator<<(std::ostream& os, InputManager::Method m);

MAA_CTRL_UNIT_NS_END
