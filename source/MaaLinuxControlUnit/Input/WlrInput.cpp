//
// Created by free on 7/14/26.
//

#include "WlrInput.h"

#include "Input/XKBKeyMap.h"
#include "MaaUtils/Logger.h"

#include <linux/input-event-codes.h>

MAA_CTRL_UNIT_NS_BEGIN
WlrInput::WlrInput(std::shared_ptr<WaylandClient> client)
    : client_(std::move(client))
    , shm_(client_->get_shm())
    , seat_(client_->get_seat())
    , output_(client_->get_output())
    , pointer_manager_(client_->get_virtual_pointer_manager())
    , keyboard_manager_(client_->get_virtual_keyboard_manager())
{
    prepare_device();
}

MaaControllerFeature WlrInput::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool WlrInput::click(int x, int y)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_up instead"
             << VAR(x) << VAR(y);
    return false;
}

bool WlrInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_move/touch_up instead"
             << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool WlrInput::touch_down(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;

    return pointer(EventPhase::Began, x, y, contact);
}

bool WlrInput::touch_move(int contact, int x, int y, int pressure)
{
    std::ignore = pressure;

    return pointer(EventPhase::Moved, x, y, contact);
}

bool WlrInput::touch_up(int contact)
{
    return pointer(EventPhase::Ended, 0, 0, contact);
}

bool WlrInput::click_key(int key)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick, "
                "use key_down/key_up instead"
             << VAR(key);
    return false;
}

bool WlrInput::input_text(const std::string& text)
{
    if (!switch_keymap(Keymap::Ascii)) {
        LogError << "Failed to switch keymap to ascii";
        return false;
    }
    for (const auto ch : text) {
        const uint64_t event_time = WaylandHelper::get_ms();
        zwp_virtual_keyboard_v1_key(keyboard_.get(), event_time, ch - 8, WL_KEYBOARD_KEY_STATE_PRESSED);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        zwp_virtual_keyboard_v1_key(keyboard_.get(), event_time, ch - 8, WL_KEYBOARD_KEY_STATE_RELEASED);
        if (!client_->process_requests()) {
            return false;
        }
    }
    return true;
}

bool WlrInput::key_down(int key)
{
    return input_key(EventPhase::Began, key);
}

bool WlrInput::key_up(int key)
{
    return input_key(EventPhase::Ended, key);
}

bool WlrInput::scroll(int dx, int dy)
{
    if (dy != 0) {
        // Wayland 的垂直滚动方向与 Maa 对外约定（dy > 0 表示向上滚动）相反，
        // 这里对 dy 取反以与 Win32 等控制器保持一致行为。
        const int step_y = -dy / 120;
        for (int i = 0; i < abs(step_y); ++i) {
            zwlr_virtual_pointer_v1_axis_discrete(
                pointer_.get(),
                WaylandHelper::get_ms(),
                WL_POINTER_AXIS_VERTICAL_SCROLL,
                wl_fixed_from_int(step_y >= 0 ? 10 : -10),
                step_y >= 0 ? 1 : -1);
            zwlr_virtual_pointer_v1_frame(pointer_.get());
            if (!client_->process_requests()) {
                return false;
            }
        }
    }

    if (dx != 0) {
        const int step_x = dx / 120;
        for (int i = 0; i < abs(step_x); ++i) {
            zwlr_virtual_pointer_v1_axis_discrete(
                pointer_.get(),
                WaylandHelper::get_ms(),
                WL_POINTER_AXIS_HORIZONTAL_SCROLL,
                wl_fixed_from_int(step_x >= 0 ? 10 : -10),
                step_x >= 0 ? 1 : -1);
            zwlr_virtual_pointer_v1_frame(pointer_.get());
            if (!client_->process_requests()) {
                return false;
            }
        }
    }
    return true;
}

bool WlrInput::pointer(EventPhase phase, int x, int y, int contact)
{
    const uint32_t event_time = WaylandHelper::get_ms();

    int btn = BTN_LEFT;
    switch (contact) {
    case 1:
        btn = BTN_RIGHT;
        break;
    case 2:
        btn = BTN_MIDDLE;
        break;
    case 0:
    default:;
    }

    switch (phase) {
    case EventPhase::Began:
        zwlr_virtual_pointer_v1_motion_absolute(pointer_.get(), event_time, x, y, screen_size_.first, screen_size_.second);
        zwlr_virtual_pointer_v1_button(pointer_.get(), event_time, btn, WL_POINTER_BUTTON_STATE_PRESSED);
        break;
    case EventPhase::Moved:
        zwlr_virtual_pointer_v1_motion_absolute(pointer_.get(), event_time, x, y, screen_size_.first, screen_size_.second);
        break;
    case EventPhase::Ended:
        zwlr_virtual_pointer_v1_button(pointer_.get(), event_time, btn, WL_POINTER_BUTTON_STATE_RELEASED);
        break;
    default:;
    }

    zwlr_virtual_pointer_v1_frame(pointer_.get());
    return client_->process_requests();
}

bool WlrInput::relative_move(int dx, int dy)
{
    const uint32_t event_time = WaylandHelper::get_ms();
    zwlr_virtual_pointer_v1_motion(pointer_.get(), event_time, wl_fixed_from_int(dx), wl_fixed_from_int(dy));
    zwlr_virtual_pointer_v1_frame(pointer_.get());
    return client_->process_requests();
}

bool WlrInput::input_key(EventPhase phase, int key)
{
    const uint64_t event_time = WaylandHelper::get_ms();
    if (!switch_keymap(Keymap::Scancode)) {
        LogError << "Failed to switch keymap to scancode";
        return false;
    }
    const int depressed_modifier = WaylandHelper::depressed_key_modifiers(key);
    const int locked_modifier = WaylandHelper::locked_key_modifiers(key);
    if (depressed_modifier != 0 || locked_modifier != 0) {
        switch (phase) {
        case EventPhase::Began:
            current_depressed_modifiers_ |= depressed_modifier;
            current_locked_modifiers_ ^= locked_modifier;
            break;
        case EventPhase::Moved:
            break;
        case EventPhase::Ended:
            current_depressed_modifiers_ &= ~depressed_modifier;
            break;
        default:;
        }
        zwp_virtual_keyboard_v1_modifiers(keyboard_.get(), current_depressed_modifiers_, 0, current_locked_modifiers_, 0);
        if (locked_modifier != 0) {
            return client_->process_requests();
        }
    }
    switch (phase) {
    case EventPhase::Began:
        zwp_virtual_keyboard_v1_key(keyboard_.get(), event_time, key, WL_KEYBOARD_KEY_STATE_PRESSED);
        break;
    case EventPhase::Moved:
        break;
    case EventPhase::Ended:
        zwp_virtual_keyboard_v1_key(keyboard_.get(), event_time, key, WL_KEYBOARD_KEY_STATE_RELEASED);
        break;
    default:;
    }
    return client_->process_requests();
}

bool WlrInput::switch_keymap(Keymap new_map)
{
    if (current_keymap_ == new_map) {
        return true;
    }

    LogDebug << "Switching keymap" << VAR(current_keymap_) << VAR(new_map);

    switch (new_map) {
    case Keymap::Unknown:
        break;
    case Keymap::Ascii:
        zwp_virtual_keyboard_v1_keymap(
            keyboard_.get(),
            WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
            ascii_keymap_buffer_->fd(),
            ascii_keymap_buffer_->size());
        current_keymap_ = Keymap::Ascii;
        break;
    case Keymap::Scancode:
        zwp_virtual_keyboard_v1_keymap(
            keyboard_.get(),
            WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
            scancode_keymap_buffer_->fd(),
            scancode_keymap_buffer_->size());
        current_keymap_ = Keymap::Scancode;
        break;
    default:
        LogWarn << "Using unknown keymap" << VAR(new_map);
    }

    return client_->process_requests();
}

bool WlrInput::prepare_device()
{
    pointer_.reset(zwlr_virtual_pointer_manager_v1_create_virtual_pointer_with_output(pointer_manager_.get(), seat_.get(), output_.get()));
    keyboard_.reset(zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(keyboard_manager_.get(), seat_.get()));
    zwlr_virtual_pointer_v1_axis_source(pointer_.get(), WL_POINTER_AXIS_SOURCE_WHEEL);
    wl_output_listener output_listener = { };
    output_listener.done = [](void* data, struct wl_output* wl_output) {
        std::ignore = wl_output;
        std::ignore = data;
        LogTrace << "End of output info";
    };
    output_listener.name = [](void* data, struct wl_output* wl_output, const char* name) {
        std::ignore = wl_output;
        std::ignore = data;
        LogTrace << "Output name" << VAR(name);
    };
    output_listener.description = [](void* data, struct wl_output* wl_output, const char* description) {
        std::ignore = wl_output;
        std::ignore = data;
        LogTrace << "Output desc" << VAR(description);
    };
    output_listener.scale = [](void* data, struct wl_output* wl_output, int32_t factor) {
        std::ignore = wl_output;
        std::ignore = data;
        LogTrace << "Output scale" << VAR(factor);
    };
    output_listener.geometry = [](void* data,
                                  struct wl_output* wl_output,
                                  int32_t x,
                                  int32_t y,
                                  int32_t physical_width,
                                  int32_t physical_height,
                                  int32_t subpixel,
                                  const char* make,
                                  const char* model,
                                  int32_t transform) {
        std::ignore = wl_output;
        std::ignore = data;
        LogTrace << "Output geometry" << VAR(x) << VAR(y) << VAR(physical_width) << VAR(physical_height) << VAR(subpixel) << VAR(make)
                 << VAR(model) << VAR(transform);
    };
    output_listener.mode = [](void* data, struct wl_output* wl_output, uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
        std::ignore = wl_output;
        const auto self = static_cast<WlrInput*>(data);
        LogTrace << "Output mode" << VAR(flags) << VAR(width) << VAR(height) << VAR(refresh);
        if (flags == WL_OUTPUT_MODE_CURRENT) {
            self->screen_size_ = { width, height };
            LogDebug << "Setting screen size" << VAR(width) << VAR(height);
        }
    };
    wl_output_add_listener(output_.get(), &output_listener, this);

    ascii_keymap_buffer_ = std::make_unique<MemfdBuffer>(kAsciiKeyMap.size());
    if (!ascii_keymap_buffer_->available()) {
        LogError << "Failed to create ascii keymap buffer";
        return false;
    }
    std::strcpy(static_cast<char*>(ascii_keymap_buffer_->ptr()), kAsciiKeyMap.data());

    scancode_keymap_buffer_ = std::make_unique<MemfdBuffer>(kScanCodeKeyMap.size());
    if (!scancode_keymap_buffer_->available()) {
        LogError << "Failed to create scancode keymap buffer";
        return false;
    }
    std::strcpy(static_cast<char*>(scancode_keymap_buffer_->ptr()), kScanCodeKeyMap.data());
    return client_->process_requests();
}

MAA_CTRL_UNIT_NS_END
