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
    std::ignore = x;
    std::ignore = y;

    return false;
}

bool MtouchHelper::swipe(int x1, int y1, int x2, int y2, int duration)
{
  std::ignore = x1;
  std::ignore = y1;
  std::ignore = x2;
  std::ignore = y2;
  std::ignore = duration;

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
