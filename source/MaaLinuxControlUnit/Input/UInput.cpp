#include "UInput.h"

#include <cerrno>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <unistd.h>
#include <utility>

#include "MaaUtils/Logger.h"
#include "Utils/AsciiToEvdev.h"

MAA_CTRL_UNIT_NS_BEGIN

UInput::UInput(std::filesystem::path device_node, int screen_width, int screen_height)
    : screen_width_(screen_width)
    , screen_height_(screen_height)
    , device_node_(std::move(device_node))
{
    open();
}

UInput::~UInput()
{
    close();
}

bool UInput::open()
{
    if (screen_width_ <= 0 || screen_height_ <= 0) {
        LogError << "Invalid screen dimensions" << VAR(screen_width_) << VAR(screen_height_);
        return false;
    }

    std::unique_lock<std::mutex> lock(mutex_);

    if (connected_) {
        LogWarn << "Already connected, closing first";
        destroy_device();
    }

    if (!create_device()) {
        LogError << "Failed to create uinput device";
        return false;
    }

    connected_ = true;
    pointer_down_ = false;
    LogInfo << "UInput device created successfully";
    return true;
}

void UInput::close()
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

bool UInput::connected() const
{
    return connected_;
}

MaaControllerFeature UInput::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool UInput::click_key(int key)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick, "
                "use key_down/key_up instead"
             << VAR(key);
    return false;
}

bool UInput::click(int x, int y)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_up instead"
             << VAR(x) << VAR(y);
    return false;
}

bool UInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_move/touch_up instead"
             << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);
    return false;
}

bool UInput::touch_down(int contact, int x, int y, int pressure)
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
    }
    else if (contact == 2) {
        btn_code = BTN_MIDDLE;
    }

    return send_pointer_down(x, y, btn_code);
}

bool UInput::touch_move(int contact, int x, int y, int pressure)
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

bool UInput::touch_up(int contact)
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
    }
    else if (contact == 2) {
        btn_code = BTN_MIDDLE;
    }

    return send_pointer_up(btn_code);
}

bool UInput::input_text(const std::string& str)
{
    LogDebug << VAR(str);

    for (const auto ch : str) {
        auto [key, need_shift] = ascii_to_evdev(ch);
        if (need_shift) {
            if (!key_down(KEY_LEFTSHIFT)) {
                return false;
            }
        }
        if (!key_down(key)) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (!key_up(key)) {
            return false;
        }
        if (need_shift) {
            if (!key_up(KEY_LEFTSHIFT)) {
                return false;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return true;
}

bool UInput::scroll(int dx, int dy)
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

bool UInput::relative_move(int dx, int dy)
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

bool UInput::key_down(int key_code)
{
    LogDebug << VAR(key_code);

    std::unique_lock<std::mutex> lock(mutex_);

    if (!connected_) {
        LogError << "Not connected";
        return false;
    }

    if (!emit_key(key_code, 1)) {
        return false;
    }
    if (!emit_syn()) {
        return false;
    }
    return true;
}

bool UInput::key_up(int key_code)
{
    LogDebug << VAR(key_code);

    std::unique_lock<std::mutex> lock(mutex_);

    if (!connected_) {
        LogError << "Not connected";
        return false;
    }

    if (!emit_key(key_code, 0)) {
        return false;
    }
    if (!emit_syn()) {
        return false;
    }
    return true;
}

std::pair<int, int> UInput::screen_size() const
{
    return { screen_width_, screen_height_ };
}

bool UInput::create_device()
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

    for (int i = 0; i < 105; i++) {
        ioctl(fd_, UI_SET_KEYBIT, i);
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
    uinput_user_dev udev = { };
    std::strncpy(udev.name, "MaaFramework KWin Virtual Input", sizeof(udev.name) - 1);
    udev.id.bustype = BUS_VIRTUAL;
    udev.id.vendor = 0x3255;
    udev.id.product = 0x7999;
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

bool UInput::destroy_device()
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

bool UInput::emit_abs(int code, int value)
{
    input_event ev = { };
    ev.type = EV_ABS;
    ev.code = code;
    ev.value = value;

    if (write(fd_, &ev, sizeof(ev)) != sizeof(ev)) {
        LogError << "Failed to write EV_ABS event" << VAR(code) << VAR(value) << VAR(errno) << VAR(std::strerror(errno));
        return false;
    }
    return true;
}

bool UInput::emit_key(int code, int value)
{
    input_event ev = { };
    ev.type = EV_KEY;
    ev.code = code;
    ev.value = value;

    if (write(fd_, &ev, sizeof(ev)) != sizeof(ev)) {
        LogError << "Failed to write EV_KEY event" << VAR(code) << VAR(value) << VAR(errno) << VAR(std::strerror(errno));
        return false;
    }
    return true;
}

bool UInput::emit_syn()
{
    input_event ev = { };
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;

    if (write(fd_, &ev, sizeof(ev)) != sizeof(ev)) {
        LogError << "Failed to write SYN_REPORT" << VAR(errno) << VAR(std::strerror(errno));
        return false;
    }
    return true;
}

bool UInput::send_pointer_down(int x, int y, int btn_code)
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

bool UInput::send_pointer_move(int x, int y)
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

bool UInput::send_pointer_up(int btn_code)
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

inline uint64_t UInput::now_ms()
{
    timespec ts = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * UINT64_C(1000) + static_cast<uint64_t>(ts.tv_nsec) / UINT64_C(1'000'000);
}

MAA_CTRL_UNIT_NS_END
