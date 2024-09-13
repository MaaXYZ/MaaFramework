#pragma once

#include "Base/UnitBase.h"
#include "MaaFramework/MaaDef.h"
#include "Utils/SafeWindows.hpp"

MAA_CTRL_UNIT_NS_BEGIN

class InputAgent : public InputBase
{
public:
    InputAgent(MaaWin32InputMethod method, HWND hWnd);
    virtual ~InputAgent() override = default;

public: // from InputBase
    virtual bool click(int x, int y) override;
    virtual bool swipe(int x1, int y1, int x2, int y2, int duration) override;

    virtual bool touch_down(int contact, int x, int y, int pressure) override;
    virtual bool touch_move(int contact, int x, int y, int pressure) override;
    virtual bool touch_up(int contact) override;

    virtual bool press_key(int key) override;
    virtual bool input_text(const std::string& text) override;

private:
    HWND hwnd_ = nullptr;

    std::shared_ptr<InputBase> input_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
