#include "MtouchHelper.h"

#include <array>
#include <cmath>
#include <format>
#include <ranges>

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

bool MtouchHelper::read_info()
{
    if (!pipe_ios_) {
        LogError << "pipe_ios_ is nullptr";
        return false;
    }

    using namespace std::chrono_literals;
    std::ignore = pipe_ios_->read_until("^", 5s);
    constexpr std::string_view kFlag = "\n";
    std::string info = pipe_ios_->read_until(kFlag, 1s);

    if (!info.ends_with(kFlag)) {
        LogError << "failed to read info";
        return false;
    }
    LogInfo << VAR(info);

    int contact = 0;
    int x = 0;
    int y = 0;
    int pressure = 0;

    std::istringstream ins(std::move(info));
    if (!(ins >> contact >> x >> y >> pressure)) {
        return false;
    }

    if (!request_display_info()) {
        LogWarn << "failed to request display info";
        return false;
    }

    bool landscape = display_width_ > display_height_;
    touch_width_ = landscape ? std::max(x, y) : std::min(x, y);
    touch_height_ = landscape ? std::min(x, y) : std::max(x, y);
    xscale_ = static_cast<double>(touch_width_) / display_width_;
    yscale_ = static_cast<double>(touch_height_) / display_height_;
    press_ = pressure;

    LogInfo << VAR(display_width_) << VAR(display_height_) << VAR(touch_width_) << VAR(touch_height_) << VAR(xscale_) << VAR(yscale_)
            << VAR(press_) << VAR(orientation_);

    return true;
}

bool MtouchHelper::request_display_info()
{
    auto resolution_opt = device_info_->request_resolution();
    if (!resolution_opt) {
        LogError << "failed to request resolution";
        return false;
    }

    display_width_ = resolution_opt->w;
    display_height_ = resolution_opt->h;
    orientation_ = resolution_opt->r;

    auto orientation_opt = device_info_->request_orientation();
    if (!orientation_opt) {
        LogWarn << "failed to request orientation";
        return true;
    }

    orientation_ = *orientation_opt;

    return true;
}

bool MtouchHelper::click(int x, int y)
{
    LogError << "deprecated" << VAR(x) << VAR(y);

    return false;
}

bool MtouchHelper::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << "deprecated" << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    return false;
}

bool MtouchHelper::touch_down(int contact, int x, int y, int pressure)
{
    if (!pipe_ios_) {
        LogError << "pipe_ios_ is nullptr";
        return false;
    }

    auto [touch_x, touch_y] = screen_to_touch(x, y);

    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(touch_x) << VAR(touch_y);

    bool ret = pipe_ios_->write(std::format(kDownFormat, contact, touch_x, touch_y, pressure));

    if (!ret) {
        LogError << "failed to write";
        return false;
    }

    return true;
}

bool MtouchHelper::touch_move(int contact, int x, int y, int pressure)
{
    if (!pipe_ios_) {
        LogError << "pipe_ios_ is nullptr";
        return false;
    }

    auto [touch_x, touch_y] = screen_to_touch(x, y);

    // LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(touch_x) << VAR(touch_y);

    bool ret = pipe_ios_->write(std::format(kMoveFormat, contact, touch_x, touch_y, pressure));

    if (!ret) {
        LogError << "failed to write";
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

    LogInfo << VAR(contact);

    bool ret = pipe_ios_->write(std::format(kUpFormat, contact));

    if (!ret) {
        LogError << "failed to write";
        return false;
    }

    return true;
}

bool MtouchHelper::parse(const json::value& config)
{
    return device_info_->parse(config);
}

MAA_CTRL_UNIT_NS_END
