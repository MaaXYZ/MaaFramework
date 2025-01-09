#include "MtouchHelper.h"

#include <array>
#include <cmath>
#include <format>
#include <ranges>

#include "Utils/Logger.h"
#include "Utils/MicroControl.hpp"

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

    std::tie(display_width_, display_height_) = *resolution_opt;

    auto orientation_opt = device_info_->request_orientation();
    if (!orientation_opt) {
        LogError << "failed to request orientation";
        return false;
    }

    orientation_ = *orientation_opt;

    return true;
}

bool MtouchHelper::click(int x, int y)
{
    if (!pipe_ios_) {
        LogError << "pipe_ios_ is nullptr";
        return false;
    }

    if (x < 0 || x >= display_width_ || y < 0 || y >= display_height_) {
        LogWarn << "click point out of range" << VAR(x) << VAR(y);
        x = std::clamp(x, 0, display_width_ - 1);
        y = std::clamp(y, 0, display_height_ - 1);
    }

    auto [touch_x, touch_y] = screen_to_touch(x, y);

    LogInfo << VAR(x) << VAR(y) << VAR(touch_x) << VAR(touch_y);

    bool ret = pipe_ios_->write(std::format(kDownFormat, 0, touch_x, touch_y, press_)) && pipe_ios_->write(std::format(kUpFormat, 0));

    if (!ret) {
        LogError << "failed to write";
        return false;
    }

    return true;
}

bool MtouchHelper::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!pipe_ios_) {
        LogError << "pipe_ios_ is nullptr";
        return false;
    }

    if (x1 < 0 || x1 >= display_width_ || y1 < 0 || y1 >= display_height_ || x2 < 0 || x2 >= display_width_ || y2 < 0
        || y2 >= display_height_) {
        LogWarn << "swipe point out of range" << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2);
        x1 = std::clamp(x1, 0, display_width_ - 1);
        y1 = std::clamp(y1, 0, display_height_ - 1);
        x2 = std::clamp(x2, 0, display_width_ - 1);
        y2 = std::clamp(y2, 0, display_height_ - 1);
    }
    if (duration <= 0) {
        LogWarn << "duration out of range" << VAR(duration);
        duration = 200;
    }

    auto [touch_x1, touch_y1] = screen_to_touch(x1, y1);
    auto [touch_x2, touch_y2] = screen_to_touch(x2, y2);

    LogInfo << VAR(x1) << VAR(y1) << VAR(touch_x1) << VAR(touch_y1) << VAR(x2) << VAR(y2) << VAR(touch_x2) << VAR(touch_y2)
            << VAR(duration);

    bool ret = true;

    micro_swipe(
        touch_x1,
        touch_y1,
        touch_x2,
        touch_y2,
        duration,
        [&](int x, int y) { ret &= pipe_ios_->write(std::format(kDownFormat, 0, x, y, press_)); },
        [&](int x, int y) { ret &= pipe_ios_->write(std::format(kMoveFormat, 0, x, y, press_)); },
        [&]([[maybe_unused]] int x, [[maybe_unused]] int y) { ret &= pipe_ios_->write(std::format(kUpFormat, 0)); });

    return ret;
}

bool MtouchHelper::multi_swipe(const std::vector<SwipeParam>& swipes)
{
    if (!pipe_ios_) {
        LogError << "pipe_ios_ is nullptr";
        return false;
    }

    std::vector<SwipeParam> correction = swipes;

    for (SwipeParam& s : correction) {
        if (s.x1 < 0 || s.x1 >= display_width_ || s.y1 < 0 || s.y1 >= display_height_ || s.x2 < 0 || s.x2 >= display_width_ || s.y2 < 0
            || s.y2 >= display_height_) {
            LogWarn << "swipe point out of range" << VAR(s.x1) << VAR(s.y1) << VAR(s.x2) << VAR(s.y2);
            s.x1 = std::clamp(s.x1, 0, display_width_ - 1);
            s.y1 = std::clamp(s.y1, 0, display_height_ - 1);
            s.x2 = std::clamp(s.x2, 0, display_width_ - 1);
            s.y2 = std::clamp(s.y2, 0, display_height_ - 1);
        }
        if (s.duration <= 0) {
            LogWarn << "duration out of range" << VAR(s.duration);
            s.duration = 200;
        }

        auto [touch_x1, touch_y1] = screen_to_touch(s.x1, s.y1);
        auto [touch_x2, touch_y2] = screen_to_touch(s.x2, s.y2);

        LogInfo << VAR(s.x1) << VAR(s.y1) << VAR(touch_x1) << VAR(touch_y1) << VAR(s.x2) << VAR(s.y2) << VAR(touch_x2) << VAR(touch_y2)
                << VAR(s.duration);

        s.x1 = touch_x1;
        s.y1 = touch_y1;
        s.x2 = touch_x2;
        s.y2 = touch_y2;
    }

    bool ret = true;

    micro_multi_swipe(
        correction,
        [&](int contact, int x, int y) { ret &= pipe_ios_->write(std::format(kDownFormat, contact, x, y, press_)); },
        [&](int contact, int x, int y) { ret &= pipe_ios_->write(std::format(kMoveFormat, contact, x, y, press_)); },
        [&](int contact, [[maybe_unused]] int x, [[maybe_unused]] int y) { ret &= pipe_ios_->write(std::format(kUpFormat, contact)); });

    return ret;
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

    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(touch_x) << VAR(touch_y);

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
