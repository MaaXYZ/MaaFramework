#pragma once

#include "Base/UnitBase.h"
#include "MaaUtils/SafeWindows.hpp"

#include <ViGEm/Client.h>

MAA_CTRL_UNIT_NS_BEGIN

class GamepadInput : public InputBase
{
public:
    GamepadInput();
    virtual ~GamepadInput() override;

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

public:
    bool set_button(WORD button, bool pressed);
    bool set_left_stick(SHORT x, SHORT y);
    bool set_right_stick(SHORT x, SHORT y);
    bool set_left_trigger(BYTE value);
    bool set_right_trigger(BYTE value);

private:
    bool load_vigem();
    void unload_vigem();
    bool init_gamepad();
    void uninit_gamepad();
    bool send_state();

    HMODULE vigem_module_ = nullptr;
    PVIGEM_CLIENT client_ = nullptr;
    PVIGEM_TARGET pad_ = nullptr;
    XUSB_REPORT report_ = {};
    bool inited_ = false;

    // Function pointers
    PFN_vigem_alloc fn_alloc_ = nullptr;
    PFN_vigem_free fn_free_ = nullptr;
    PFN_vigem_connect fn_connect_ = nullptr;
    PFN_vigem_disconnect fn_disconnect_ = nullptr;
    PFN_vigem_target_x360_alloc fn_target_x360_alloc_ = nullptr;
    PFN_vigem_target_free fn_target_free_ = nullptr;
    PFN_vigem_target_add fn_target_add_ = nullptr;
    PFN_vigem_target_remove fn_target_remove_ = nullptr;
    PFN_vigem_target_x360_update fn_target_x360_update_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
