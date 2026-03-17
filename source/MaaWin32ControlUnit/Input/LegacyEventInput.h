#pragma once

#include "ControlUnit/ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class LegacyEventInput : public InputBase
{
public:
    LegacyEventInput(HWND hwnd, bool block_input = false)
        : hwnd_(hwnd)
        , block_input_(block_input)
    {
    }

    virtual ~LegacyEventInput() override;

public: // from InputBase
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

    virtual void inactive() override;

private:
    void ensure_foreground();

    void check_and_block_input();
    void unblock_input();

    HWND hwnd_ = nullptr;
    const bool block_input_ = false;
};

MAA_CTRL_UNIT_NS_END

