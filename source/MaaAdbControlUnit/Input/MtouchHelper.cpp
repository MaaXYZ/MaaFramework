#include "MtouchHelper.h"

#include <array>
#include <cmath>
#include <format>
#include <ranges>

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

bool MtouchHelper::read_info(int swidth, int sheight, int orientation)
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

    screen_width_ = swidth;
    screen_height_ = sheight;
    bool landscape = screen_width_ > screen_height_;
    touch_width_ = landscape ? std::max(x, y) : std::min(x, y);
    touch_height_ = landscape ? std::min(x, y) : std::max(x, y);
    xscale_ = static_cast<double>(touch_width_) / swidth;
    yscale_ = static_cast<double>(touch_height_) / sheight;
    press_ = pressure;
    orientation_ = orientation;

    LogInfo << VAR(screen_width_) << VAR(screen_height_) << VAR(touch_width_) << VAR(touch_height_)
            << VAR(xscale_) << VAR(yscale_) << VAR(press_) << VAR(orientation_);

    return true;
}

bool MtouchHelper::click(int x, int y)
{
    if (!pipe_ios_) {
        LogError << "pipe_ios_ is nullptr";
        return false;
    }

    if (x < 0 || x >= screen_width_ || y < 0 || y >= screen_height_) {
        LogWarn << "click point out of range" << VAR(x) << VAR(y);
        x = std::clamp(x, 0, screen_width_ - 1);
        y = std::clamp(y, 0, screen_height_ - 1);
    }

    auto [touch_x, touch_y] = screen_to_touch(x, y);

    LogInfo << VAR(x) << VAR(y) << VAR(touch_x) << VAR(touch_y);

    bool ret = pipe_ios_->write(std::format(kDownFormat, 0, touch_x, touch_y, press_))
               && pipe_ios_->write(std::format(kUpFormat, 0));

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

    if (x1 < 0 || x1 >= screen_width_ || y1 < 0 || y1 >= screen_height_ || x2 < 0
        || x2 >= screen_width_ || y2 < 0 || y2 >= screen_height_) {
        LogWarn << "swipe point out of range" << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2);
        x1 = std::clamp(x1, 0, screen_width_ - 1);
        y1 = std::clamp(y1, 0, screen_height_ - 1);
        x2 = std::clamp(x2, 0, screen_width_ - 1);
        y2 = std::clamp(y2, 0, screen_height_ - 1);
    }
    if (duration <= 0) {
        LogWarn << "duration out of range" << VAR(duration);
        duration = 500;
    }

    auto [touch_x1, touch_y1] = screen_to_touch(x1, y1);
    auto [touch_x2, touch_y2] = screen_to_touch(x2, y2);

    LogInfo << VAR(x1) << VAR(y1) << VAR(touch_x1) << VAR(touch_y1) << VAR(x2) << VAR(y2)
            << VAR(touch_x2) << VAR(touch_y2) << VAR(duration);

    auto start = std::chrono::steady_clock::now();
    auto now = start;
    bool ret = true;
    ret &= pipe_ios_->write(std::format(kDownFormat, 0, touch_x1, touch_y1, press_));
    if (!ret) {
        LogError << "write error";
        return false;
    }

    constexpr double kInterval = 10; // ms
    const double steps = duration / kInterval;
    const double x_step_len = (x2 - x1) / steps;
    const double y_step_len = (y2 - y1) / steps;
    const std::chrono::milliseconds delay(static_cast<int>(kInterval));

    for (int i = 0; i < steps; ++i) {
        auto [tx, ty] = screen_to_touch(x1 + i * x_step_len, y1 + i * y_step_len);
        std::this_thread::sleep_until(now + delay);
        now = std::chrono::steady_clock::now();

        ret &= pipe_ios_->write(std::format(kMoveFormat, 0, tx, ty, press_));
        if (!ret) {
            LogWarn << "write error";
        }
    }

    std::this_thread::sleep_until(now + delay);
    now = std::chrono::steady_clock::now();
    ret &= pipe_ios_->write(std::format(kMoveFormat, 0, touch_x2, touch_y2, press_));

    std::this_thread::sleep_until(now + delay);
    now = std::chrono::steady_clock::now();
    ret &= pipe_ios_->write(std::format(kUpFormat, 0));

    if (!ret) {
        LogError << "failed to write";
        return false;
    }

    return true;
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

MAA_CTRL_UNIT_NS_END