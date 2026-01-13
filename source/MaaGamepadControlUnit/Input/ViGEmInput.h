#pragma once

#include <memory>
#include <string>

#include "MaaFramework/MaaDef.h"
#include "MaaUtils/LibraryHolder.h"
#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class ViGEmInput : public LibraryHolder<ViGEmInput>
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

private:
    // ViGEm function types
    using FnVigemAlloc = void* (*)();
    using FnVigemFree = void (*)(void*);
    using FnVigemConnect = uint32_t (*)(void*);
    using FnVigemDisconnect = void (*)(void*);
    using FnVigemTargetAlloc = void* (*)();
    using FnVigemTargetFree = void (*)(void*);
    using FnVigemTargetAdd = uint32_t (*)(void*, void*);
    using FnVigemTargetRemove = uint32_t (*)(void*, void*);

#pragma pack(push, 1)

    struct XusbReport
    {
        uint16_t wButtons;
        uint8_t bLeftTrigger;
        uint8_t bRightTrigger;
        int16_t sThumbLX;
        int16_t sThumbLY;
        int16_t sThumbRX;
        int16_t sThumbRY;
    };

    struct Ds4Report
    {
        uint8_t bThumbLX;
        uint8_t bThumbLY;
        uint8_t bThumbRX;
        uint8_t bThumbRY;
        uint16_t wButtons;
        uint8_t bSpecial;
        uint8_t bTriggerL;
        uint8_t bTriggerR;
    };

#pragma pack(pop)

    using FnVigemTargetX360Update = uint32_t (*)(void*, void*, XusbReport);
    using FnVigemTargetDs4Update = uint32_t (*)(void*, void*, Ds4Report);

    // Function pointers
    boost::function<FnVigemAlloc> fn_vigem_alloc_;
    boost::function<FnVigemFree> fn_vigem_free_;
    boost::function<FnVigemConnect> fn_vigem_connect_;
    boost::function<FnVigemDisconnect> fn_vigem_disconnect_;
    boost::function<FnVigemTargetAlloc> fn_vigem_target_x360_alloc_;
    boost::function<FnVigemTargetAlloc> fn_vigem_target_ds4_alloc_;
    boost::function<FnVigemTargetFree> fn_vigem_target_free_;
    boost::function<FnVigemTargetAdd> fn_vigem_target_add_;
    boost::function<FnVigemTargetRemove> fn_vigem_target_remove_;
    boost::function<FnVigemTargetX360Update> fn_vigem_target_x360_update_;
    boost::function<FnVigemTargetDs4Update> fn_vigem_target_ds4_update_;
};

MAA_CTRL_UNIT_NS_END
