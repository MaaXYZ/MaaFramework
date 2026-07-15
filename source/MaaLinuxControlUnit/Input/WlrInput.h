#pragma once

#include "Base/MemfdBuffer.h"
#include "Base/UnitBase.h"
#include "Common/Conf.h"
#include "Wayland/WaylandClient.h"
#include "Wayland/WaylandHelper.h"

MAA_CTRL_UNIT_NS_BEGIN

class WlrInput : public InputBase
{
public:
    explicit WlrInput(std::shared_ptr<WaylandClient> client)
        : client_(std::move(client))
        , shm_(client_->get_shm())
        , seat_(client_->get_seat())
        , output_(client_->get_output())
        , pointer_manager_(client_->get_virtual_pointer_manager())
        , keyboard_manager_(client_->get_virtual_keyboard_manager())
    {
    }

    virtual ~WlrInput() override = default;

public:
    MaaControllerFeature get_features() const override;
    bool click(int x, int y) override;
    bool swipe(int x1, int y1, int x2, int y2, int duration) override;
    bool touch_down(int contact, int x, int y, int pressure) override;
    bool touch_move(int contact, int x, int y, int pressure) override;
    bool touch_up(int contact) override;
    bool click_key(int key) override;
    bool input_text(const std::string& text) override;
    bool key_down(int key) override;
    bool key_up(int key) override;
    bool scroll(int dx, int dy) override;

private:
    enum class Keymap : uint8_t
    {
        Unknown = 0,
        Ascii = 1,
        Scancode = 2,
    };

    bool pointer(EventPhase phase, int x, int y, int contact);
    bool relative_move(int dx, int dy);
    bool input_key(EventPhase phase, int key);
    bool switch_keymap(Keymap new_map);
    bool prepare_device();

    std::shared_ptr<WaylandClient> client_;
    std::shared_ptr<wl_shm> shm_;
    std::shared_ptr<wl_seat> seat_;
    std::shared_ptr<wl_output> output_;
    std::shared_ptr<zwlr_virtual_pointer_manager_v1> pointer_manager_;
    std::shared_ptr<zwp_virtual_keyboard_manager_v1> keyboard_manager_;

    std::unique_ptr<zwlr_virtual_pointer_v1> pointer_;
    std::unique_ptr<MemfdBuffer> ascii_keymap_buffer_;
    std::unique_ptr<MemfdBuffer> scancode_keymap_buffer_;
    std::unique_ptr<zwp_virtual_keyboard_v1> keyboard_;

    Keymap current_keymap_ = Keymap::Unknown;
    int current_depressed_modifiers_ = 0;
    int current_locked_modifiers_ = 0;

    std::pair<int, int> screen_size_ { 0, 0 };
};

MAA_CTRL_UNIT_NS_END
