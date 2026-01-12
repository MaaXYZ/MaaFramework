#pragma once

#include <memory>
#include <string>

#include "MaaFramework/MaaDef.h"
#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class ViGEmInput
{
public:
    explicit ViGEmInput(MaaGamepadType type);
    ~ViGEmInput();

    ViGEmInput(const ViGEmInput&) = delete;
    ViGEmInput& operator=(const ViGEmInput&) = delete;

    bool connect();

    bool connected() const { return connected_; }

    MaaGamepadType type() const { return type_; }

    // Digital button operations (for click_key/key_down/key_up)
    bool press_button(int button);
    bool release_button(int button);
    bool click_button(int button);

    // Analog stick operations (for touch_down/touch_move/touch_up with contact 0/1)
    // x, y: -32768 ~ 32767
    bool set_left_stick(int x, int y);
    bool set_right_stick(int x, int y);
    bool reset_left_stick();
    bool reset_right_stick();

    // Trigger operations (for touch_down/touch_move/touch_up with contact 2/3)
    // value: 0 ~ 255
    bool set_left_trigger(int value);
    bool set_right_trigger(int value);
    bool reset_left_trigger();
    bool reset_right_trigger();

    // Reset all inputs to default state
    void reset();

private:
    bool init_vigem();
    void cleanup_vigem();
    bool update_report();

    // Dynamic loading support
    bool load_vigem_library();
    void unload_vigem_library();

private:
    MaaGamepadType type_ = MaaGamepadType_Xbox360;
    bool connected_ = false;

    // ViGEm handles (stored as void* for dynamic loading)
    void* vigem_client_ = nullptr;
    void* vigem_target_ = nullptr;

    // Current state
    union
    {
        struct
        {
            uint16_t wButtons;
            uint8_t bLeftTrigger;
            uint8_t bRightTrigger;
            int16_t sThumbLX;
            int16_t sThumbLY;
            int16_t sThumbRX;
            int16_t sThumbRY;
        } xbox;

        struct
        {
            uint8_t bThumbLX;
            uint8_t bThumbLY;
            uint8_t bThumbRX;
            uint8_t bThumbRY;
            uint16_t wButtons;
            uint8_t bSpecial;
            uint8_t bTriggerL;
            uint8_t bTriggerR;
        } ds4;
    } state_ {};

    // Dynamic library handle
    HMODULE vigem_dll_ = nullptr;

    // Function pointers for dynamic loading
    void* fn_vigem_alloc_ = nullptr;
    void* fn_vigem_free_ = nullptr;
    void* fn_vigem_connect_ = nullptr;
    void* fn_vigem_disconnect_ = nullptr;
    void* fn_vigem_target_x360_alloc_ = nullptr;
    void* fn_vigem_target_ds4_alloc_ = nullptr;
    void* fn_vigem_target_free_ = nullptr;
    void* fn_vigem_target_add_ = nullptr;
    void* fn_vigem_target_remove_ = nullptr;
    void* fn_vigem_target_x360_update_ = nullptr;
    void* fn_vigem_target_ds4_update_ = nullptr;
};

MAA_CTRL_UNIT_NS_END
