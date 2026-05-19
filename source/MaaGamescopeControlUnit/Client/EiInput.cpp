#include "EiInput.h"

#include <cerrno>
#include <linux/input-event-codes.h>
#include <poll.h>

#include <libei.h>

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

EiInput::EiInput(std::string eis_socket_path)
    : eis_socket_path_(std::move(eis_socket_path))
{
    LogFunc << VAR(eis_socket_path_);
}

EiInput::~EiInput()
{
    LogFunc;
    shutdown();
}

bool EiInput::init()
{
    LogFunc;

    auto cleanup = [this]() {
        if (device_) {
            ei_device_unref(device_);
            device_ = nullptr;
        }
        if (ei_) {
            ei_unref(ei_);
            ei_ = nullptr;
        }
        return false;
    };

    ei_ = ei_new_sender(nullptr);
    if (!ei_) {
        LogError << "ei_new_sender failed";
        return false;
    }

    ei_configure_name(ei_, "MaaFramework");

    int rc = ei_setup_backend_socket(ei_, eis_socket_path_.c_str());
    if (rc < 0) {
        LogError << "ei_setup_backend_socket failed" << VAR(rc) << VAR(eis_socket_path_);
        return cleanup();
    }

    ei_fd_ = ei_get_fd(ei_);
    if (ei_fd_ < 0) {
        LogError << "ei_get_fd failed";
        return cleanup();
    }

    constexpr int kPollMs = 100;
    constexpr int kMaxAttempts = 30;

    for (int i = 0; i < kMaxAttempts && !connected_; ++i) {
        int n = poll_and_dispatch(kPollMs);
        if (n < 0) {
            LogError << "poll failed during init";
            return cleanup();
        }
    }

    if (!connected_) {
        LogError << (device_ ? "EiInput init timed out" : "No device with keyboard and pointer capabilities");
        return cleanup();
    }

    ei_device_start_emulating(device_, ++emul_seq_);
    ei_device_frame(device_, ei_now(ei_));

    LogInfo << "EiInput ready";
    return true;
}

void EiInput::shutdown()
{
    if (!ei_) {
        return;
    }

    connected_ = false;
    if (device_) {
        ei_device_stop_emulating(device_);
        ei_device_unref(device_);
    }
    device_ = nullptr;
    ei_disconnect(ei_);
    ei_unref(ei_);
    ei_ = nullptr;
    ei_fd_ = -1;
}

int EiInput::poll_and_dispatch(int timeout_ms)
{
    pollfd pfd = { ei_fd_, POLLIN, 0 };
    int n = poll(&pfd, 1, timeout_ms);
    while (n < 0 && errno == EINTR) {
        n = poll(&pfd, 1, timeout_ms);
    }
    if (n > 0 && (pfd.revents & POLLIN)) {
        ei_dispatch(ei_);
        while (auto* event = ei_get_event(ei_)) {
            handle_event(event);
            ei_event_unref(event);
        }
    }
    return n;
}

void EiInput::handle_event(struct ei_event* event)
{
    switch (ei_event_get_type(event)) {
    case EI_EVENT_CONNECT:
        LogInfo << "EI_EVENT_CONNECT";
        break;
    case EI_EVENT_SEAT_ADDED: {
        struct ei_seat* seat = ei_event_get_seat(event);
        ei_seat_bind_capabilities(
            seat,
            EI_DEVICE_CAP_POINTER,
            EI_DEVICE_CAP_POINTER_ABSOLUTE,
            EI_DEVICE_CAP_KEYBOARD,
            EI_DEVICE_CAP_BUTTON,
            EI_DEVICE_CAP_SCROLL,
            EI_DEVICE_CAP_TEXT,
            nullptr);
        break;
    }
    case EI_EVENT_DEVICE_ADDED: {
        struct ei_device* dev = ei_event_get_device(event);
        if (ei_device_has_capability(dev, EI_DEVICE_CAP_KEYBOARD) && ei_device_has_capability(dev, EI_DEVICE_CAP_POINTER)) {
            if (device_) {
                ei_device_unref(device_);
            }
            device_ = ei_device_ref(dev);
        }
        break;
    }
    case EI_EVENT_DEVICE_RESUMED:
        if (ei_event_get_device(event) == device_) {
            connected_ = true;
        }
        LogInfo << "EI_EVENT_DEVICE_RESUMED";
        break;
    case EI_EVENT_KEYBOARD_MODIFIERS:
        break;
    case EI_EVENT_DEVICE_PAUSED:
        LogInfo << "EI_EVENT_DEVICE_PAUSED";
        break;
    case EI_EVENT_DEVICE_REMOVED:
        if (ei_event_get_device(event) == device_) {
            ei_device_unref(device_);
            device_ = nullptr;
            connected_ = false;
        }
        LogInfo << "EI_EVENT_DEVICE_REMOVED";
        break;
    case EI_EVENT_SEAT_REMOVED:
        LogInfo << "EI_EVENT_SEAT_REMOVED";
        break;
    case EI_EVENT_DISCONNECT:
        LogWarn << "EI_EVENT_DISCONNECT";
        if (device_) {
            ei_device_unref(device_);
            device_ = nullptr;
        }
        connected_ = false;
        break;
    default:
        LogTrace << "Unhandled ei event" << VAR(ei_event_get_type(event));
        break;
    }
}

template <typename F>
bool EiInput::send(F&& emit)
{
    if (!device_ || !ei_) {
        LogError << "send called without a valid device/ei";
        return false;
    }
    while (poll_and_dispatch(0) > 0) {
    }
    emit(device_);
    ei_device_frame(device_, ei_now(ei_));
    return true;
}

bool EiInput::pointer(EventPhase phase, int x, int y, int contact)
{
    int btn = (contact == 1) ? BTN_LEFT : (contact == 2) ? BTN_RIGHT : (contact == 3) ? BTN_MIDDLE : BTN_LEFT;

    return send([phase, x, y, btn](struct ei_device* d) {
        switch (phase) {
        case EventPhase::Began:
            ei_device_pointer_motion_absolute(d, static_cast<double>(x), static_cast<double>(y));
            ei_device_button_button(d, btn, true);
            break;
        case EventPhase::Moved:
            ei_device_pointer_motion_absolute(d, static_cast<double>(x), static_cast<double>(y));
            break;
        case EventPhase::Ended:
            ei_device_button_button(d, btn, false);
            break;
        }
    });
}

bool EiInput::keyboard_key(EventPhase phase, int evdev_key)
{
    return send([phase, evdev_key](struct ei_device* d) { ei_device_keyboard_key(d, evdev_key, phase != EventPhase::Ended); });
}

bool EiInput::text_utf8(const std::string& text)
{
    return send([&text](struct ei_device* d) { ei_device_text_utf8(d, text.c_str()); });
}

bool EiInput::relative_move(int dx, int dy)
{
    return send([dx, dy](struct ei_device* d) { ei_device_pointer_motion(d, static_cast<double>(dx), static_cast<double>(dy)); });
}

bool EiInput::scroll(int dx, int dy)
{
    return send([dx, dy](struct ei_device* d) { ei_device_scroll_delta(d, static_cast<double>(dx), static_cast<double>(dy)); });
}

MAA_CTRL_UNIT_NS_END
