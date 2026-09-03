#pragma once

#include <optional>
#include <utility>

#include "MaaControlUnit/ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "MaaUtils/SafeWindows.hpp"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class InterceptionInput : public InputBase
{
public:
    explicit InterceptionInput(HWND hwnd);
    virtual ~InterceptionInput() override;

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
    struct KeyboardStroke
    {
        uint16_t unit_id = 0;
        uint16_t code = 0;
        uint16_t state = 0;
        uint16_t reserved = 0;
        uint32_t information = 0;
    };

    struct MouseStroke
    {
        uint16_t unit_id = 0;
        uint16_t flags = 0;
        uint16_t button_flags = 0;
        uint16_t button_data = 0;
        uint32_t raw_buttons = 0;
        int32_t x = 0;
        int32_t y = 0;
        uint32_t information = 0;
    };

    bool ensure_mouse_ready();
    bool ensure_keyboard_ready();
    bool initialize_mouse_device();
    bool initialize_keyboard_device();
    void destroy_mouse_device();
    void destroy_keyboard_device();

    bool ensure_foreground();
    bool send_mouse_stroke(const MouseStroke& stroke);
    bool send_keyboard_stroke(const KeyboardStroke& stroke);
    bool move_to_client_point(int x, int y);
    bool send_button(int contact, bool button_down);
    bool send_scroll_axis(int delta, bool horizontal);
    bool send_key(int key, bool key_up);

    std::pair<int, int> get_target_pos() const;
    std::optional<POINT> client_to_screen_point(int x, int y) const;
    std::optional<std::pair<int32_t, int32_t>> to_interception_coordinate(const POINT& point) const;

private:
    HWND hwnd_ = nullptr;
    HANDLE mouse_device_handle_ = INVALID_HANDLE_VALUE;
    HANDLE mouse_event_handle_ = nullptr;
    int mouse_device_index_ = -1;
    HANDLE keyboard_device_handle_ = INVALID_HANDLE_VALUE;
    HANDLE keyboard_event_handle_ = nullptr;
    int keyboard_device_index_ = -1;

    std::pair<int, int> last_pos_ { 0, 0 };
    bool last_pos_set_ = false;
};

MAA_CTRL_UNIT_NS_END
