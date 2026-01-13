#include "ViGEmInput.h"

#include <algorithm>

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

using namespace vigem;

ViGEmInput::ViGEmInput(MaaGamepadType type)
    : type_(type)
    , state_(create_gamepad_state(type))
{
}

ViGEmInput::~ViGEmInput()
{
    cleanup_vigem();
    unload_vigem_library();
}

bool ViGEmInput::load_vigem_library()
{
    if (!load_library("ViGEmClient")) {
        LogError << "Failed to load ViGEmClient library";
        return false;
    }

    fn_vigem_alloc_ = get_function<FnVigemAlloc>("vigem_alloc");
    fn_vigem_free_ = get_function<FnVigemFree>("vigem_free");
    fn_vigem_connect_ = get_function<FnVigemConnect>("vigem_connect");
    fn_vigem_disconnect_ = get_function<FnVigemDisconnect>("vigem_disconnect");
    fn_vigem_target_x360_alloc_ = get_function<FnVigemTargetAlloc>("vigem_target_x360_alloc");
    fn_vigem_target_ds4_alloc_ = get_function<FnVigemTargetAlloc>("vigem_target_ds4_alloc");
    fn_vigem_target_free_ = get_function<FnVigemTargetFree>("vigem_target_free");
    fn_vigem_target_add_ = get_function<FnVigemTargetAdd>("vigem_target_add");
    fn_vigem_target_remove_ = get_function<FnVigemTargetRemove>("vigem_target_remove");
    fn_vigem_target_x360_update_ = get_function<FnVigemTargetX360Update>("vigem_target_x360_update");
    fn_vigem_target_ds4_update_ = get_function<FnVigemTargetDs4Update>("vigem_target_ds4_update");

    if (!fn_vigem_alloc_ || !fn_vigem_free_ || !fn_vigem_connect_ || !fn_vigem_disconnect_ || !fn_vigem_target_free_
        || !fn_vigem_target_add_ || !fn_vigem_target_remove_) {
        LogError << "Failed to get ViGEm function pointers";
        unload_vigem_library();
        return false;
    }

    if (type_ == MaaGamepadType_Xbox360) {
        if (!fn_vigem_target_x360_alloc_ || !fn_vigem_target_x360_update_) {
            LogError << "Failed to get Xbox 360 function pointers";
            unload_vigem_library();
            return false;
        }
    }
    else {
        if (!fn_vigem_target_ds4_alloc_ || !fn_vigem_target_ds4_update_) {
            LogError << "Failed to get DS4 function pointers";
            unload_vigem_library();
            return false;
        }
    }

    LogInfo << "ViGEmClient library loaded successfully";
    return true;
}

void ViGEmInput::unload_vigem_library()
{
    fn_vigem_alloc_.clear();
    fn_vigem_free_.clear();
    fn_vigem_connect_.clear();
    fn_vigem_disconnect_.clear();
    fn_vigem_target_x360_alloc_.clear();
    fn_vigem_target_ds4_alloc_.clear();
    fn_vigem_target_free_.clear();
    fn_vigem_target_add_.clear();
    fn_vigem_target_remove_.clear();
    fn_vigem_target_x360_update_.clear();
    fn_vigem_target_ds4_update_.clear();

    unload_library();
}

void* ViGEmInput::alloc_target()
{
    if (type_ == MaaGamepadType_Xbox360) {
        return fn_vigem_target_x360_alloc_();
    }
    return fn_vigem_target_ds4_alloc_();
}

bool ViGEmInput::init_vigem()
{
    if (!load_vigem_library()) {
        return false;
    }

    vigem_client_ = fn_vigem_alloc_();
    if (!vigem_client_) {
        LogError << "Failed to allocate ViGEm client";
        return false;
    }

    auto ret = fn_vigem_connect_(vigem_client_);
    if (ret != VIGEM_ERROR_NONE) {
        LogError << "Failed to connect to ViGEm bus, error:" << std::hex << ret;
        cleanup_vigem();
        return false;
    }

    vigem_target_ = alloc_target();
    if (!vigem_target_) {
        LogError << "Failed to allocate ViGEm target";
        cleanup_vigem();
        return false;
    }

    ret = fn_vigem_target_add_(vigem_client_, vigem_target_);
    if (ret != VIGEM_ERROR_NONE) {
        LogError << "Failed to add ViGEm target, error:" << std::hex << ret;
        cleanup_vigem();
        return false;
    }

    LogInfo << "ViGEm gamepad connected, type:" << (type_ == MaaGamepadType_Xbox360 ? "Xbox360" : "DualShock4");
    return true;
}

void ViGEmInput::cleanup_vigem()
{
    if (vigem_target_ && vigem_client_ && fn_vigem_target_remove_) {
        fn_vigem_target_remove_(vigem_client_, vigem_target_);
    }

    if (vigem_target_ && fn_vigem_target_free_) {
        fn_vigem_target_free_(vigem_target_);
        vigem_target_ = nullptr;
    }

    if (vigem_client_) {
        if (fn_vigem_disconnect_) {
            fn_vigem_disconnect_(vigem_client_);
        }
        if (fn_vigem_free_) {
            fn_vigem_free_(vigem_client_);
        }
        vigem_client_ = nullptr;
    }
}

bool ViGEmInput::connect()
{
    if (connected_) {
        return true;
    }

    connected_ = init_vigem();
    return connected_;
}

bool ViGEmInput::update_report()
{
    if (!connected_ || !vigem_client_ || !vigem_target_) {
        return false;
    }

    uint32_t ret = state_->update(vigem_client_, vigem_target_, fn_vigem_target_x360_update_, fn_vigem_target_ds4_update_);

    if (ret != VIGEM_ERROR_NONE) {
        LogError << "Failed to update ViGEm report, error:" << std::hex << ret;
        return false;
    }

    return true;
}

bool ViGEmInput::press_button(int button)
{
    if (!connected_) {
        return false;
    }

    state_->press_button(button);
    return update_report();
}

bool ViGEmInput::release_button(int button)
{
    if (!connected_) {
        return false;
    }

    state_->release_button(button);
    return update_report();
}

bool ViGEmInput::set_left_stick(int x, int y)
{
    if (!connected_) {
        return false;
    }

    state_->set_left_stick(x, y);
    return update_report();
}

bool ViGEmInput::set_right_stick(int x, int y)
{
    if (!connected_) {
        return false;
    }

    state_->set_right_stick(x, y);
    return update_report();
}

bool ViGEmInput::reset_left_stick()
{
    return set_left_stick(0, 0);
}

bool ViGEmInput::reset_right_stick()
{
    return set_right_stick(0, 0);
}

bool ViGEmInput::set_left_trigger(int value)
{
    if (!connected_) {
        return false;
    }

    state_->set_left_trigger(static_cast<uint8_t>(std::clamp(value, 0, 255)));
    return update_report();
}

bool ViGEmInput::set_right_trigger(int value)
{
    if (!connected_) {
        return false;
    }

    state_->set_right_trigger(static_cast<uint8_t>(std::clamp(value, 0, 255)));
    return update_report();
}

bool ViGEmInput::reset_left_trigger()
{
    return set_left_trigger(0);
}

bool ViGEmInput::reset_right_trigger()
{
    return set_right_trigger(0);
}

void ViGEmInput::reset()
{
    state_->reset();

    if (connected_) {
        update_report();
    }
}

MAA_CTRL_UNIT_NS_END
