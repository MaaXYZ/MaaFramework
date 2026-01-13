#pragma once

#include <memory>
#include <string>

#include "MaaFramework/MaaDef.h"
#include "MaaUtils/LibraryHolder.h"
#include "MaaUtils/SafeWindows.hpp"

#include "GamepadState.h"
#include "ViGEmTypes.h"

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

    bool press_button(int button);
    bool release_button(int button);
    bool click_button(int button);

    bool set_left_stick(int x, int y);
    bool set_right_stick(int x, int y);
    bool reset_left_stick();
    bool reset_right_stick();

    bool set_left_trigger(int value);
    bool set_right_trigger(int value);
    bool reset_left_trigger();
    bool reset_right_trigger();

    void reset();

private:
    bool init_vigem();
    void cleanup_vigem();
    bool update_report();

    void* alloc_target();

    bool load_vigem_library();
    void unload_vigem_library();

private:
    MaaGamepadType type_ = MaaGamepadType_Xbox360;
    bool connected_ = false;

    void* vigem_client_ = nullptr;
    void* vigem_target_ = nullptr;

    std::unique_ptr<GamepadState> state_;

private:
    boost::function<vigem::FnVigemAlloc> fn_vigem_alloc_;
    boost::function<vigem::FnVigemFree> fn_vigem_free_;
    boost::function<vigem::FnVigemConnect> fn_vigem_connect_;
    boost::function<vigem::FnVigemDisconnect> fn_vigem_disconnect_;
    boost::function<vigem::FnVigemTargetAlloc> fn_vigem_target_x360_alloc_;
    boost::function<vigem::FnVigemTargetAlloc> fn_vigem_target_ds4_alloc_;
    boost::function<vigem::FnVigemTargetFree> fn_vigem_target_free_;
    boost::function<vigem::FnVigemTargetAdd> fn_vigem_target_add_;
    boost::function<vigem::FnVigemTargetRemove> fn_vigem_target_remove_;
    boost::function<vigem::FnVigemTargetX360Update> fn_vigem_target_x360_update_;
    boost::function<vigem::FnVigemTargetDs4Update> fn_vigem_target_ds4_update_;
};

MAA_CTRL_UNIT_NS_END
