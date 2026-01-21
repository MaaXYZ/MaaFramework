#pragma once

#include <memory>

#include <boost/function.hpp>

#include "MaaFramework/MaaDef.h"
#include "ViGEmTypes.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class GamepadState
{
public:
    virtual ~GamepadState() = default;

    virtual void press_button(int button) = 0;
    virtual void release_button(int button) = 0;
    virtual void set_left_stick(int x, int y) = 0;
    virtual void set_right_stick(int x, int y) = 0;
    virtual void set_left_trigger(uint8_t value) = 0;
    virtual void set_right_trigger(uint8_t value) = 0;
    virtual void reset() = 0;

    virtual uint32_t update(
        void* client,
        void* target,
        const boost::function<vigem::FnVigemTargetX360Update>& x360_update,
        const boost::function<vigem::FnVigemTargetDs4Update>& ds4_update) = 0;
};

class XboxGamepadState : public GamepadState
{
public:
    void press_button(int button) override;
    void release_button(int button) override;
    void set_left_stick(int x, int y) override;
    void set_right_stick(int x, int y) override;
    void set_left_trigger(uint8_t value) override;
    void set_right_trigger(uint8_t value) override;
    void reset() override;

    uint32_t update(
        void* client,
        void* target,
        const boost::function<vigem::FnVigemTargetX360Update>& x360_update,
        const boost::function<vigem::FnVigemTargetDs4Update>& ds4_update) override;

private:
    vigem::XusbReport report_;
};

class Ds4GamepadState : public GamepadState
{
public:
    void press_button(int button) override;
    void release_button(int button) override;
    void set_left_stick(int x, int y) override;
    void set_right_stick(int x, int y) override;
    void set_left_trigger(uint8_t value) override;
    void set_right_trigger(uint8_t value) override;
    void reset() override;

    uint32_t update(
        void* client,
        void* target,
        const boost::function<vigem::FnVigemTargetX360Update>& x360_update,
        const boost::function<vigem::FnVigemTargetDs4Update>& ds4_update) override;

private:
    static uint16_t convert_button(int button);
    static uint8_t convert_special(int button);
    void update_dpad(int button, bool pressed);
    uint8_t compute_dpad_value() const;

private:
    vigem::Ds4Report report_;
    uint8_t dpad_state_ = 0;
};

std::unique_ptr<GamepadState> create_gamepad_state(MaaGamepadType type);

MAA_CTRL_UNIT_NS_END
