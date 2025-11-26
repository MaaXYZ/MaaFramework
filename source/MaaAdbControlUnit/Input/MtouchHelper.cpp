#include "MtouchHelper.h"

#include <array>
#include <chrono>
#include <cmath>
#include <format>
#include <ranges>
#include <thread>

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

std::string MtouchHelper::type_name() const
{
    return typeid(this).name();
}

bool MtouchHelper::read_info()
{
    if (!pipe_ios_) {
        LogError << type_name() << "pipe_ios_ is nullptr";
        return false;
    }

    using namespace std::chrono_literals;
    std::ignore = pipe_ios_->read_until("^", 5s);
    constexpr std::string_view kFlag = "\n";
    std::string info = pipe_ios_->read_until(kFlag, 1s);

    if (!info.ends_with(kFlag)) {
        LogError << type_name() << "failed to read info";
        return false;
    }
    LogInfo << type_name() << VAR(info);

    int contact = 0;
    int x = 0;
    int y = 0;
    int pressure = 0;

    std::istringstream ins(std::move(info));
    if (!(ins >> contact >> x >> y >> pressure)) {
        return false;
    }

    if (!request_display_info()) {
        LogWarn << type_name() << "failed to request display info";
        return false;
    }

    bool landscape = display_width_ > display_height_;
    touch_width_ = landscape ? std::max(x, y) : std::min(x, y);
    touch_height_ = landscape ? std::min(x, y) : std::max(x, y);
    xscale_ = static_cast<double>(touch_width_) / display_width_;
    yscale_ = static_cast<double>(touch_height_) / display_height_;
    press_ = pressure;

    LogInfo << type_name() << VAR(display_width_) << VAR(display_height_) << VAR(touch_width_) << VAR(touch_height_) << VAR(xscale_)
            << VAR(yscale_) << VAR(press_) << VAR(orientation_);

    return true;
}

bool MtouchHelper::request_display_info()
{
    auto resolution_opt = device_info_->request_resolution();
    if (!resolution_opt) {
        LogError << type_name() << "failed to request resolution";
        return false;
    }

    display_width_ = resolution_opt->w;
    display_height_ = resolution_opt->h;
    orientation_ = resolution_opt->r;

    auto orientation_opt = device_info_->request_orientation();
    if (!orientation_opt) {
        LogWarn << type_name() << "failed to request orientation";
        return true;
    }

    orientation_ = *orientation_opt;

    return true;
}

bool MtouchHelper::click(int x, int y)
{
    LogError << type_name() << "deprecated" << VAR(x) << VAR(y);

    return false;
}

bool MtouchHelper::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << type_name() << "deprecated" << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    return false;
}

bool MtouchHelper::touch_down(int contact, int x, int y, int pressure)
{
    if (!pipe_ios_) {
        LogError << type_name() << "pipe_ios_ is nullptr";
        return false;
    }

    auto [touch_x, touch_y] = screen_to_touch(x, y);

    LogInfo << type_name() << VAR(contact) << VAR(x) << VAR(y) << VAR(touch_x) << VAR(touch_y);

    bool ret = pipe_ios_->write(std::format(kDownFormat, contact, touch_x, touch_y, pressure));

    if (!ret) {
        LogError << type_name() << "failed to write";
        return false;
    }

    return true;
}

bool MtouchHelper::touch_move(int contact, int x, int y, int pressure)
{
    if (!pipe_ios_) {
        LogError << type_name() << "pipe_ios_ is nullptr";
        return false;
    }

    auto [touch_x, touch_y] = screen_to_touch(x, y);

    // LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(touch_x) << VAR(touch_y);

    bool ret = pipe_ios_->write(std::format(kMoveFormat, contact, touch_x, touch_y, pressure));

    if (!ret) {
        LogError << type_name() << "failed to write";
        return false;
    }

    return true;
}

bool MtouchHelper::touch_up(int contact)
{
    if (!pipe_ios_) {
        LogError << "pipe_ios_ is nullptr";
        return false;
    }

    LogInfo << type_name() << VAR(contact);

    bool ret = pipe_ios_->write(std::format(kUpFormat, contact));

    if (!ret) {
        LogError << type_name() << "failed to write";
        return false;
    }

    return true;
}

bool MtouchHelper::parse(const json::value& config)
{
    return device_info_->parse(config);
}

bool MtouchHelper::scroll(int x, int y, int dx, int dy, int duration)
{
    // For Adb, simulate scroll using swipe
    LogInfo << type_name() << VAR(x) << VAR(y) << VAR(dx) << VAR(dy) << VAR(duration);

    // Scale the delta values to reasonable pixel distances
    constexpr int kScrollScale = 10;
    int x2 = x + dx * kScrollScale;
    int y2 = y + dy * kScrollScale;

    // Perform touch operations to simulate scroll
    if (!touch_down(0, x, y, 1)) {
        return false;
    }

    // Move in steps for smoother scrolling
    constexpr int kSteps = 10;
    for (int i = 1; i <= kSteps; ++i) {
        int current_x = x + (x2 - x) * i / kSteps;
        int current_y = y + (y2 - y) * i / kSteps;
        if (!touch_move(0, current_x, current_y, 1)) {
            touch_up(0);
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(duration / kSteps));
    }

    return touch_up(0);
}

MAA_CTRL_UNIT_NS_END
