#include "WaylandClient.h"

#include "MemfdBuffer.h"
#include "WaylandProtocol.h"

#include "MaaUtils/Logger.h"

#include <linux/input-event-codes.h>

MAA_CTRL_UNIT_NS_BEGIN

#define WHEEL_DELTA 15

WaylandClient::WaylandClient()
{
}

WaylandClient::~WaylandClient()
{
    close();
}

bool WaylandClient::connect(const std::filesystem::path& socket_path)
{
    if (socket_path_ != socket_path) {
        LogInfo << "Closing old socket" << socket_path_;
        close();
        socket_path_ = socket_path;
    }

    return open();
}

bool WaylandClient::open()
{
    LogInfo << "Connecting to socket" << VAR(socket_path_);
    if (display_) {
        return true;
    }

    display_.reset(wl_display_connect(socket_path_.c_str()));
    if (!display_) {
        LogError << "Failed to connect wayland socket: " << socket_path_;
        return false;
    }

    if (!bind_protocol()) {
        LogError << "Failed to bind protocols";
        close();
        return false;
    }

    if (!prepare_device()) {
        LogError << "Failed to create devices";
        return false;
    }
    return true;
}

bool WaylandClient::bind_protocol()
{
    registry_.reset(wl_display_get_registry(display_.get()));
    if (!registry_) {
        LogError << "Failed to get registry";
        return false;
    }

    registry_listener_.global = [](void* data, struct wl_registry* registry, uint32_t id, const char* interface, uint32_t version) {
        const auto self = static_cast<WaylandClient*>(data);

#define BIND(interface_type, global_member)                                                                                            \
    if (std::string_view(interface) == #interface_type) {                                                                              \
        LogDebug << "Bind protocol" << VAR(interface);                                                                                 \
        self->global_member.reset(static_cast<interface_type*>(wl_registry_bind(registry, id, &interface_type##_interface, version))); \
        return;                                                                                                                        \
    }

        BIND(wl_output, output_);
        BIND(wl_shm, shm_);
        BIND(wl_seat, seat_);
        BIND(zwlr_screencopy_manager_v1, screencopy_manager_);
        BIND(zwp_virtual_keyboard_manager_v1, keyboard_manager_);
        BIND(zwlr_virtual_pointer_manager_v1, pointer_manager_);

#undef BIND
    };
    registry_listener_.global_remove =
        []([[maybe_unused]] void* data, [[maybe_unused]] struct wl_registry* registry, [[maybe_unused]] uint32_t name) {
            // nop
        };
    wl_registry_add_listener(registry_.get(), &registry_listener_, this);
    if (!process_requests()) {
        return false;
    }
    if (!output_ && !shm_) {
        LogError << "Compositor doesn't support wayland core protocol";
        return false;
    }
    if (!screencopy_manager_) {
        LogError << "Compositor doesn't support wlr_screencopy_unstable_v1 protocol";
        return false;
    }
    if (!keyboard_manager_) {
        LogError << "Compositor doesn't support virtual_keyboard_unstable_v1 protocol";
        return false;
    }
    if (!pointer_manager_) {
        LogError << "Compositor doesn't support wlr_virtual_pointer_unstable_v1 protocol";
        return false;
    }

    return true;
}

bool WaylandClient::prepare_device()
{
    pointer_.reset(zwlr_virtual_pointer_manager_v1_create_virtual_pointer_with_output(pointer_manager_.get(), seat_.get(), output_.get()));
    keyboard_.reset(zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(keyboard_manager_.get(), seat_.get()));
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
        const auto self = static_cast<WaylandClient*>(data);
        LogTrace << "Output mode" << VAR(flags) << VAR(width) << VAR(height) << VAR(refresh);
        if (flags == WL_OUTPUT_MODE_CURRENT) {
            self->screen_size_ = { width, height };
            LogDebug << "Setting screen size" << VAR(width) << VAR(height);
        }
    };
    wl_output_add_listener(output_.get(), &output_listener, this);
    zwlr_virtual_pointer_v1_axis_source(pointer_.get(), WL_POINTER_AXIS_SOURCE_WHEEL);
    // TODO: prepare xkb data
    return process_requests();
}

bool WaylandClient::process_requests() const
{
    if (const int result = wl_display_roundtrip(display_.get()); result == -1) {
        LogError << "Failed to process requests";
        return false;
    }

    return true;
}

bool WaylandClient::screencap(void** buffer, uint32_t& width, uint32_t& height, uint32_t& format)
{
    std::unique_ptr<zwlr_screencopy_frame_v1> screencopy_frame;
    screencopy_frame.reset(zwlr_screencopy_manager_v1_capture_output(screencopy_manager_.get(), 0, output_.get()));

    zwlr_screencopy_frame_v1_listener frame_listener = { };
    frame_listener.buffer = [](void* data, zwlr_screencopy_frame_v1* frame, uint32_t format, uint32_t w, uint32_t h, uint32_t stride) {
        LogTrace << "Received new buffer: " << VAR(format) << VAR(w) << VAR(h) << VAR(stride);
        const auto self = static_cast<WaylandClient*>(data);
        if (!self->check_buffer(format, w, h, stride)) {
            if (!self->create_buffer(format, w, h, stride)) {
                LogError << "Failed to prepare buffer";
                return;
            }
        }
        zwlr_screencopy_frame_v1_copy(frame, self->buffer_obj_.get());
    };
    frame_listener.ready =
        [](void* data, zwlr_screencopy_frame_v1* frame, uint32_t tv_sec_hi, uint32_t tv_sec_lo, [[maybe_unused]] uint32_t tv_nsec) {
            std::ignore = frame; // do not destroy manual
            const auto self = static_cast<WaylandClient*>(data);
            int64_t seconds = (static_cast<int64_t>(tv_sec_hi) << 32) + tv_sec_lo;
            LogTrace << "Captured frame on " << VAR(seconds);
            self->capture_waiting_ = false;
            self->capture_successful_ = true;
        };
    frame_listener.failed = [](void* data, zwlr_screencopy_frame_v1* frame) {
        std::ignore = frame; // do not destroy manual
        const auto self = static_cast<WaylandClient*>(data);
        self->capture_waiting_ = false;
        self->capture_successful_ = false;
        LogError << "Failed to capture frame";
    };
    frame_listener.damage = [](void* data, zwlr_screencopy_frame_v1* frame, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
        std::ignore = data;
        std::ignore = frame;
        LogTrace << "Buffer damage: " << VAR(x) << VAR(y) << VAR(w) << VAR(h);
        // nop
    };
    frame_listener.buffer_done = [](void* data, zwlr_screencopy_frame_v1* frame) {
        std::ignore = data;
        std::ignore = frame;
        LogTrace << "All buffer sent";
        // nop
    };
    frame_listener.linux_dmabuf = [](void* data, zwlr_screencopy_frame_v1* frame, uint32_t format, uint32_t w, uint32_t h) {
        std::ignore = data;
        std::ignore = frame;
        LogTrace << "Received new dmabuf buffer: " << VAR(format) << VAR(w) << VAR(h);
        // nop
    };
    frame_listener.flags = [](void* data, struct zwlr_screencopy_frame_v1* frame, uint32_t flags) {
        std::ignore = data;
        std::ignore = frame;
        LogTrace << "Capture flag: " << VAR(flags);
    };
    capture_waiting_ = true;
    capture_successful_ = false;
    zwlr_screencopy_frame_v1_add_listener(screencopy_frame.get(), &frame_listener, this);

    while (capture_waiting_) {
        if (!process_requests()) {
            return false;
        }
    }

    width = buffer_->width();
    height = buffer_->height();
    format = buffer_format_;
    *buffer = buffer_->ptr();
    return capture_successful_;
}

void WaylandClient::close()
{
    close_buffer();
    LogInfo << "Closing the wayland socket";
}

bool WaylandClient::connected() const
{
    return display_ != nullptr;
}

bool WaylandClient::pointer(EventPhase phase, int x, int y, int contact)
{
    const uint32_t event_time = get_ms();

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
    return process_requests();
}

bool WaylandClient::scroll(int dx, int dy)
{
    // TODO
    std::ignore = dx;
    std::ignore = dy;
    if (dy != 0) {
        zwlr_virtual_pointer_v1_axis_discrete(
            pointer_.get(),
            get_ms(),
            WL_POINTER_AXIS_VERTICAL_SCROLL,
            wl_fixed_from_int(WHEEL_DELTA),
            dy);
    }

    if (dx != 0) {
        zwlr_virtual_pointer_v1_axis_discrete(
            pointer_.get(),
            get_ms(),
            WL_POINTER_AXIS_HORIZONTAL_SCROLL,
            wl_fixed_from_int(WHEEL_DELTA),
            dx);
    }
    zwlr_virtual_pointer_v1_frame(pointer_.get());
    return process_requests();
}

bool WaylandClient::input(EventPhase phase, int key)
{
    const uint64_t event_time = get_ms();
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
    return process_requests();
}

std::pair<int, int> WaylandClient::screen_size() const
{
    return screen_size_;
}

bool WaylandClient::check_buffer(int format, int width, int height, int stride) const
{
    if (!buffer_obj_ && !buffer_) {
        return false;
    }
    if (width != buffer_->width() || height != buffer_->height() || stride != buffer_->stride() || format != buffer_format_) {
        return false;
    }
    return true;
}

bool WaylandClient::create_buffer(int format, int width, int height, int stride)
{
    if (!close_buffer()) {
        LogError << "Failed to close old buffer";
    }
    buffer_ = std::make_unique<MemfdBuffer>(width, height, stride);
    if (!buffer_->available()) {
        LogError << "Failed to close shm buffer";
        return false;
    }
    buffer_format_ = format;
    shm_pool_.reset(wl_shm_create_pool(shm_.get(), buffer_->fd(), buffer_->size()));
    buffer_obj_.reset(wl_shm_pool_create_buffer(shm_pool_.get(), 0, width, height, stride, format));
    if (!process_requests()) {
        LogError << "Failed to create buffer objects";
        return false;
    }
    return true;
}

bool WaylandClient::close_buffer()
{
    if (!buffer_) {
        return true;
    }
    LogDebug << "Closing buffer" << VAR(buffer_);
    buffer_obj_.reset();
    shm_pool_.reset();
    if (!process_requests()) {
        LogError << "Failed to destroy buffer objects";
        return false;
    }
    buffer_.reset();
    return true;
}

MAA_CTRL_UNIT_NS_END
