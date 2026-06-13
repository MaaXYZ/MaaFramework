#if defined(__linux__) && !defined(__ANDROID__)

#include "UInputController.h"
#include "AsciiToEvdev.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

UInputController::UInputController()
{
}

UInputController::~UInputController()
{
    close();
}

bool UInputController::open(const std::filesystem::path& device_node, int screen_width, int screen_height)
{
    LogInfo << VAR(device_node) << VAR(screen_width) << VAR(screen_height);

    if (screen_width <= 0 || screen_height <= 0) {
        LogError << "Invalid screen dimensions" << VAR(screen_width) << VAR(screen_height);
        return false;
    }

    std::unique_lock<std::mutex> lock(mutex_);

    if (connected_) {
        LogWarn << "Already connected, closing first";
        destroy_device();
    }

    device_node_ = device_node;
    screen_width_ = screen_width;
    screen_height_ = screen_height;

    if (!create_device()) {
        LogError << "Failed to create uinput device";
        return false;
    }

    connected_ = true;
    pointer_down_ = false;
    LogInfo << "UInput device created successfully";
    return true;
}

void UInputController::close()
{
    std::unique_lock<std::mutex> lock(mutex_);

    if (!connected_) {
        return;
    }

    destroy_device();
    connected_ = false;
    pointer_down_ = false;
    LogInfo << "UInput device closed";
}

bool UInputController::connected() const
{
    return connected_;
}

bool UInputController::click(int x, int y)
{
    LogDebug << VAR(x) << VAR(y);

    std::unique_lock<std::mutex> lock(mutex_);

    if (!connected_) {
        LogError << "Not connected";
        return false;
    }

    if (!send_pointer_down(x, y)) {
        LogError << "Failed to send pointer down";
        return false;
    }

    // Small delay to simulate a real click
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    if (!send_pointer_up()) {
        LogError << "Failed to send pointer up";
        return false;
    }

    return true;
}

bool UInputController::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogDebug << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    std::unique_lock<std::mutex> lock(mutex_);

    if (!connected_) {
        LogError << "Not connected";
        return false;
    }

    constexpr int kSteps = 20; // Number of interpolation steps

    if (!send_pointer_down(x1, y1)) {
        LogError << "Failed to send pointer down";
        return false;
    }

    int step_duration = duration / kSteps;
    if (step_duration < 1) {
        step_duration = 1;
    }

    for (int i = 1; i <= kSteps; ++i) {
        float t = static_cast<float>(i) / kSteps;
        int cx = static_cast<int>(std::round(x1 + (x2 - x1) * t));
        int cy = static_cast<int>(std::round(y1 + (y2 - y1) * t));

        if (!send_pointer_move(cx, cy)) {
            LogError << "Failed to send pointer move at step" << i;
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(step_duration));
    }

    if (!send_pointer_up()) {
        LogError << "Failed to send pointer up";
        return false;
    }

    return true;
}

bool UInputController::touch_down(int contact, int x, int y, int pressure)
{
    LogDebug << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    std::unique_lock<std::mutex> lock(mutex_);

    if (!connected_) {
        LogError << "Not connected";
        return false;
    }

    int btn_code = BTN_LEFT;
    if (contact == 1) {
        btn_code = BTN_RIGHT;
    } else if (contact == 2) {
        btn_code = BTN_MIDDLE;
    }

    return send_pointer_down(x, y, btn_code);
}

bool UInputController::touch_move(int contact, int x, int y, int pressure)
{
    LogDebug << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    std::unique_lock<std::mutex> lock(mutex_);

    if (!connected_) {
        LogError << "Not connected";
        return false;
    }

    if (!pointer_down_) {
        LogError << "Pointer is not down, cannot move";
        return false;
    }

    return send_pointer_move(x, y);
}

bool UInputController::touch_up(int contact)
{
    LogDebug << VAR(contact);

    std::unique_lock<std::mutex> lock(mutex_);

    if (!connected_) {
        LogError << "Not connected";
        return false;
    }

    if (!pointer_down_) {
        LogError << "Pointer is not down";
        return false;
    }

    int btn_code = BTN_LEFT;
    if (contact == 1) {
        btn_code = BTN_RIGHT;
    } else if (contact == 2) {
        btn_code = BTN_MIDDLE;
    }

    return send_pointer_up(btn_code);
}

bool UInputController::scroll(int dx, int dy)
{
    LogDebug << VAR(dx) << VAR(dy);

    std::unique_lock<std::mutex> lock(mutex_);

    if (!connected_) {
        LogError << "Not connected";
        return false;
    }

    struct input_event ev;

    if (dy != 0) {
        std::memset(&ev, 0, sizeof(ev));
        ev.type = EV_REL;
        ev.code = REL_WHEEL;
        ev.value = -dy; // Negative for natural scrolling direction
        if (write(fd_, &ev, sizeof(ev)) != sizeof(ev)) {
            LogError << "Failed to write REL_WHEEL event" << VAR(errno) << VAR(std::strerror(errno));
            return false;
        }
    }

    if (dx != 0) {
        std::memset(&ev, 0, sizeof(ev));
        ev.type = EV_REL;
        ev.code = REL_HWHEEL;
        ev.value = dx;
        if (write(fd_, &ev, sizeof(ev)) != sizeof(ev)) {
            LogError << "Failed to write REL_HWHEEL event" << VAR(errno) << VAR(std::strerror(errno));
            return false;
        }
    }

    if (!emit_syn()) {
        LogError << "Failed to emit SYN_REPORT for scroll";
        return false;
    }

    return true;
}

bool UInputController::relative_move(int dx, int dy)
{
    LogDebug << VAR(dx) << VAR(dy);

    std::unique_lock<std::mutex> lock(mutex_);

    if (!connected_) {
        LogError << "Not connected";
        return false;
    }

    struct input_event ev;

    if (dx != 0) {
        std::memset(&ev, 0, sizeof(ev));
        ev.type = EV_REL;
        ev.code = REL_X;
        ev.value = dx;
        if (write(fd_, &ev, sizeof(ev)) != sizeof(ev)) {
            LogError << "Failed to write REL_X event" << VAR(errno) << VAR(std::strerror(errno));
            return false;
        }
    }

    if (dy != 0) {
        std::memset(&ev, 0, sizeof(ev));
        ev.type = EV_REL;
        ev.code = REL_Y;
        ev.value = dy;
        if (write(fd_, &ev, sizeof(ev)) != sizeof(ev)) {
            LogError << "Failed to write REL_Y event" << VAR(errno) << VAR(std::strerror(errno));
            return false;
        }
    }

    if (!emit_syn()) {
        LogError << "Failed to emit SYN_REPORT for relative move";
        return false;
    }

    return true;
}

bool UInputController::key_down(int key_code)
{
    LogDebug << VAR(key_code);

    std::unique_lock<std::mutex> lock(mutex_);

    if (!connected_) {
        LogError << "Not connected";
        return false;
    }

    int linux_key = maa_to_linux_keycode(key_code);
    if (!emit_key(linux_key, 1)) {
        return false;
    }
    if (!emit_syn()) {
        return false;
    }
    return true;
}

bool UInputController::key_up(int key_code)
{
    LogDebug << VAR(key_code);

    std::unique_lock<std::mutex> lock(mutex_);

    if (!connected_) {
        LogError << "Not connected";
        return false;
    }

    int linux_key = maa_to_linux_keycode(key_code);
    if (!emit_key(linux_key, 0)) {
        return false;
    }
    if (!emit_syn()) {
        return false;
    }
    return true;
}

std::pair<int, int> UInputController::screen_size() const
{
    return { screen_width_, screen_height_ };
}

int UInputController::maa_to_linux_keycode(int key_code)
{
    return ascii_to_evdev(key_code);
}

bool UInputController::create_device()
{
    LogInfo << "Creating uinput device at" << VAR(device_node_);

    fd_ = ::open(device_node_.c_str(), O_WRONLY | O_NONBLOCK);
    if (fd_ < 0) {
        LogError << "Failed to open" << VAR(device_node_) << VAR(errno) << VAR(std::strerror(errno));
        return false;
    }

    // Enable event types
    if (ioctl(fd_, UI_SET_EVBIT, EV_KEY) < 0) {
        LogError << "Failed to set EV_KEY" << VAR(errno) << VAR(std::strerror(errno));
        ::close(fd_);
        fd_ = -1;
        return false;
    }
    if (ioctl(fd_, UI_SET_EVBIT, EV_ABS) < 0) {
        LogError << "Failed to set EV_ABS" << VAR(errno) << VAR(std::strerror(errno));
        ::close(fd_);
        fd_ = -1;
        return false;
    }
    if (ioctl(fd_, UI_SET_EVBIT, EV_REL) < 0) {
        LogWarn << "Failed to set EV_REL, relative events may not work" << VAR(errno) << VAR(std::strerror(errno));
        // Non-fatal, continue
    }
    if (ioctl(fd_, UI_SET_EVBIT, EV_SYN) < 0) {
        LogError << "Failed to set EV_SYN" << VAR(errno) << VAR(std::strerror(errno));
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    // Register BTN_LEFT for pointer click support
    if (ioctl(fd_, UI_SET_KEYBIT, BTN_LEFT) < 0) {
        LogError << "Failed to set BTN_LEFT" << VAR(errno) << VAR(std::strerror(errno));
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    // Register alphabet keys (KEY_A .. KEY_Z)
    for (int k = KEY_A; k <= KEY_Z; ++k) {
        ioctl(fd_, UI_SET_KEYBIT, k);
    }

    // Register digit keys (KEY_1 .. KEY_0)
    for (int k = KEY_1; k <= KEY_0; ++k) {
        ioctl(fd_, UI_SET_KEYBIT, k);
    }

    // Register common additional keys
    const int kCommonKeys[] = {
        KEY_ENTER,
        KEY_BACKSPACE,
        KEY_TAB,
        KEY_ESC,
        KEY_SPACE,
        KEY_MINUS,
        KEY_EQUAL,
        KEY_LEFTBRACE,
        KEY_RIGHTBRACE,
        KEY_BACKSLASH,
        KEY_SEMICOLON,
        KEY_APOSTROPHE,
        KEY_GRAVE,
        KEY_COMMA,
        KEY_DOT,
        KEY_SLASH,
        KEY_LEFTSHIFT,
        KEY_LEFTCTRL,
        KEY_LEFTALT,
        KEY_LEFTMETA,
        KEY_RIGHTSHIFT,
        KEY_RIGHTCTRL,
        KEY_RIGHTALT,
        KEY_RIGHTMETA,
        KEY_CAPSLOCK,
        KEY_F1,
        KEY_F2,
        KEY_F3,
        KEY_F4,
        KEY_F5,
        KEY_F6,
        KEY_F7,
        KEY_F8,
        KEY_F9,
        KEY_F10,
        KEY_F11,
        KEY_F12,
        KEY_UP,
        KEY_DOWN,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_HOME,
        KEY_END,
        KEY_PAGEUP,
        KEY_PAGEDOWN,
        KEY_INSERT,
        KEY_DELETE,
    };
    for (int k : kCommonKeys) {
        ioctl(fd_, UI_SET_KEYBIT, k);
    }

    // Register ONLY ABS_X and ABS_Y — absolute positioning axes.
    // No ABS_MT_* axes at all, so udev will NOT classify this as a touchscreen/touchpad.
    if (ioctl(fd_, UI_SET_ABSBIT, ABS_X) < 0) {
        LogError << "Failed to set ABS_X" << VAR(errno) << VAR(std::strerror(errno));
        ::close(fd_);
        fd_ = -1;
        return false;
    }
    if (ioctl(fd_, UI_SET_ABSBIT, ABS_Y) < 0) {
        LogError << "Failed to set ABS_Y" << VAR(errno) << VAR(std::strerror(errno));
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    // Enable relative axes (for scroll and relative_move)
    if (ioctl(fd_, UI_SET_RELBIT, REL_X) < 0) {
        LogWarn << "Failed to set REL_X" << VAR(errno) << VAR(std::strerror(errno));
    }
    if (ioctl(fd_, UI_SET_RELBIT, REL_Y) < 0) {
        LogWarn << "Failed to set REL_Y" << VAR(errno) << VAR(std::strerror(errno));
    }
    if (ioctl(fd_, UI_SET_RELBIT, REL_WHEEL) < 0) {
        LogWarn << "Failed to set REL_WHEEL" << VAR(errno) << VAR(std::strerror(errno));
    }
    if (ioctl(fd_, UI_SET_RELBIT, REL_HWHEEL) < 0) {
        LogWarn << "Failed to set REL_HWHEEL" << VAR(errno) << VAR(std::strerror(errno));
    }

    // Configure the uinput device using the traditional uinput_user_dev struct.
    struct uinput_user_dev udev;
    std::memset(&udev, 0, sizeof(udev));
    std::strncpy(udev.name, "MaaFramework KWin Virtual Input", sizeof(udev.name) - 1);
    udev.id.bustype = BUS_USB;
    udev.id.vendor = 0x1234;
    udev.id.product = 0x5678;
    udev.id.version = 1;

    // Set ABS min/max for the two absolute axes we registered.
    udev.absmin[ABS_X] = 0;
    udev.absmax[ABS_X] = screen_width_ - 1;
    udev.absfuzz[ABS_X] = 0;
    udev.absflat[ABS_X] = 0;

    udev.absmin[ABS_Y] = 0;
    udev.absmax[ABS_Y] = screen_height_ - 1;
    udev.absfuzz[ABS_Y] = 0;
    udev.absflat[ABS_Y] = 0;

    // Write the uinput_user_dev struct to the fd.
    ssize_t written = write(fd_, &udev, sizeof(udev));
    if (written != static_cast<ssize_t>(sizeof(udev))) {
        LogError << "Failed to write uinput_user_dev" << VAR(errno) << VAR(std::strerror(errno));
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    // Create the device
    if (ioctl(fd_, UI_DEV_CREATE) < 0) {
        LogError << "Failed to create uinput device" << VAR(errno) << VAR(std::strerror(errno));
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    LogInfo << "UInput device created successfully";
    return true;
}

bool UInputController::destroy_device()
{
    LogInfo << "Destroying uinput device";

    if (fd_ < 0) {
        return true;
    }

    // Destroy the uinput device
    if (ioctl(fd_, UI_DEV_DESTROY) < 0) {
        LogWarn << "Failed to destroy uinput device" << VAR(errno) << VAR(std::strerror(errno));
        // Continue anyway
    }

    ::close(fd_);
    fd_ = -1;

    return true;
}

bool UInputController::emit_abs(int code, int value)
{
    struct input_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.type = EV_ABS;
    ev.code = code;
    ev.value = value;

    if (write(fd_, &ev, sizeof(ev)) != sizeof(ev)) {
        LogError << "Failed to write EV_ABS event" << VAR(code) << VAR(value) << VAR(errno) << VAR(std::strerror(errno));
        return false;
    }
    return true;
}

bool UInputController::emit_key(int code, int value)
{
    struct input_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.type = EV_KEY;
    ev.code = code;
    ev.value = value;

    if (write(fd_, &ev, sizeof(ev)) != sizeof(ev)) {
        LogError << "Failed to write EV_KEY event" << VAR(code) << VAR(value) << VAR(errno) << VAR(std::strerror(errno));
        return false;
    }
    return true;
}

bool UInputController::emit_syn()
{
    struct input_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;

    if (write(fd_, &ev, sizeof(ev)) != sizeof(ev)) {
        LogError << "Failed to write SYN_REPORT" << VAR(errno) << VAR(std::strerror(errno));
        return false;
    }
    return true;
}

bool UInputController::send_pointer_down(int x, int y, int btn_code)
{
    // Move to absolute position first
    if (!emit_abs(ABS_X, x)) {
        return false;
    }
    if (!emit_abs(ABS_Y, y)) {
        return false;
    }
    // Press the specified button
    if (!emit_key(btn_code, 1)) {
        return false;
    }
    if (!emit_syn()) {
        return false;
    }

    pointer_down_ = true;
    return true;
}

bool UInputController::send_pointer_move(int x, int y)
{
    // Move to new absolute position (button state unchanged)
    if (!emit_abs(ABS_X, x)) {
        return false;
    }
    if (!emit_abs(ABS_Y, y)) {
        return false;
    }
    if (!emit_syn()) {
        return false;
    }

    return true;
}

bool UInputController::send_pointer_up(int btn_code)
{
    // Release the specified button
    if (!emit_key(btn_code, 0)) {
        return false;
    }
    if (!emit_syn()) {
        return false;
    }

    pointer_down_ = false;
    return true;
}

uint64_t UInputController::now_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

MAA_CTRL_UNIT_NS_END

#endif // __linux__ && !__ANDROID__
