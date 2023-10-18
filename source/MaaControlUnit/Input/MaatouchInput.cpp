#include "MaatouchInput.h"

#include "Utils/Format.hpp"
#include "Utils/Logger.h"
#include "Utils/Ranges.hpp"

#include <array>
#include <cmath>

MAA_CTRL_UNIT_NS_BEGIN

bool MaatouchInput::parse(const json::value& config)
{
    auto popt = config.find<json::object>("prebuilt");
    if (!popt) {
        LogError << "Cannot find entry prebuilt";
        return false;
    }

    auto mopt = popt->find<json::object>("maatouch");
    if (!mopt) {
        LogError << "Cannot find entry prebuilt.maatouch";
        return false;
    }

    {
        auto opt = mopt->find<json::value>("package");
        if (!opt) {
            LogError << "Cannot find entry prebuilt.maatouch.package";
            return false;
        }

        if (!opt->is_string()) {
            return false;
        }

        package_name_ = opt->as_string();
    }

    return invoke_app_->parse(config);
}

bool MaatouchInput::init(int swidth, int sheight, int orientation)
{
    LogFunc;

    if (!invoke_app_->init()) {
        return false;
    }

    const auto bin_path = agent_path_ / path("universal") / path("maatouch");
    if (!invoke_app_->push(bin_path)) {
        return false;
    }

    if (!invoke_app_->chmod()) {
        return false;
    }

    auto start_time = std::chrono::steady_clock::now();
    bool timeout = false;
    auto check_time = [&]() {
        timeout = duration_since(start_time) > std::chrono::seconds(10);
        return !timeout;
    };

    shell_handler_ = invoke_app_->invoke_app(package_name_);
    if (!shell_handler_) {
        return false;
    }

    std::string prev;
    std::string info;
    while (check_time()) {
        auto str = prev + shell_handler_->read(5);
        LogDebug << "output:" << str;
        if (str.empty()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
        auto pos = str.find('^');
        if (pos == std::string::npos) {
            continue;
        }
        auto rpos = str.find('\n', pos);
        if (rpos == std::string::npos) {
            prev = str; // 也许还得再读点?
            continue;
        }

        info = str.substr(pos + 1, rpos - pos - 1);
        break;
    }
    if (timeout) {
        LogError << "read maatouch info timeout";
        return false;
    }

    LogInfo << "maatouch info:" << info;
    string_trim_(info);

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
    touch_width_ = x;
    touch_height_ = y;
    xscale_ = double(touch_width_) / swidth;
    yscale_ = double(touch_height_) / sheight;
    press_ = pressure;
    orientation_ = orientation;

    LogInfo << VAR(screen_width_) << VAR(screen_height_) << VAR(touch_width_) << VAR(touch_height_) << VAR(xscale_)
            << VAR(yscale_) << VAR(press_) << VAR(orientation_);

    return true;
}

void MaatouchInput::set_wh(int swidth, int sheight, int orientation)
{
    init(swidth, sheight, orientation);
}

bool MaatouchInput::click(int x, int y)
{
    if (!shell_handler_) {
        LogError << "shell handler not ready";
        return false;
    }

    if (x < 0 || x >= screen_width_ || y < 0 || y >= screen_height_) {
        LogWarn << "click point out of range" << VAR(x) << VAR(y);
        x = std::clamp(x, 0, screen_width_ - 1);
        y = std::clamp(y, 0, screen_height_ - 1);
    }

    auto [touch_x, touch_y] = screen_to_touch(x, y);

    LogInfo << VAR(x) << VAR(y) << VAR(touch_x) << VAR(touch_y);

    bool ret = shell_handler_->write(MAA_FMT::format("d {} {} {} {}\nc\n", 0, touch_x, touch_y, press_)) &&
               shell_handler_->write(MAA_FMT::format("u {}\nc\n", 0));

    if (!ret) {
        LogError << "failed to write";
        return false;
    }

    return ret;
}

bool MaatouchInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!shell_handler_) {
        return false;
    }

    if (x1 < 0 || x1 >= screen_width_ || y1 < 0 || y1 >= screen_height_ || x2 < 0 || x2 >= screen_width_ || y2 < 0 ||
        y2 >= screen_height_) {
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

    LogInfo << VAR(x1) << VAR(y1) << VAR(touch_x1) << VAR(touch_y1) << VAR(x2) << VAR(y2) << VAR(touch_x2)
            << VAR(touch_y2) << VAR(duration);

    auto start = std::chrono::steady_clock::now();
    auto now = start;
    bool ret = true;
    ret &= shell_handler_->write(MAA_FMT::format("d {} {} {} {}\nc\n", 0, touch_x1, touch_y1, press_));
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
        auto [tx, ty] = screen_to_touch(x1 + steps * x_step_len, y1 + steps * y_step_len);
        std::this_thread::sleep_until(now + delay);
        now = std::chrono::steady_clock::now();

        ret &= shell_handler_->write(MAA_FMT::format("m {} {} {} {}\nc\n", 0, tx, ty, press_));
        if (!ret) {
            LogWarn << "write error";
        }
    }

    std::this_thread::sleep_until(now + delay);
    now = std::chrono::steady_clock::now();
    ret &= shell_handler_->write(MAA_FMT::format("m {} {} {} {}\nc\n", 0, touch_x2, touch_y2, press_));

    std::this_thread::sleep_until(now + delay);
    now = std::chrono::steady_clock::now();
    ret &= shell_handler_->write(MAA_FMT::format("u {}\nc\n", 0));

    if (!ret) {
        LogError << "failed to write";
        return false;
    }

    return ret;
}

bool MaatouchInput::press_key(int key)
{
    if (!shell_handler_) {
        return false;
    }

    bool ret = shell_handler_->write(MAA_FMT::format("k {} d\nc\n", key)) &&
               shell_handler_->write(MAA_FMT::format("k {} u\nc\n", key));

    if (!ret) {
        LogError << "failed to write";
        return false;
    }

    return ret;
}

bool MaatouchInput::touch_down(int contact, int x, int y, int pressure)
{
    if (!shell_handler_) {
        LogError << "shell handler not ready";
        return false;
    }

    auto [touch_x, touch_y] = screen_to_touch(x, y);

    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(touch_x) << VAR(touch_y);

    bool ret = shell_handler_->write(MAA_FMT::format("d {} {} {} {}\nc\n", contact, touch_x, touch_y, pressure));

    if (!ret) {
        LogError << "failed to write";
        return false;
    }

    return ret;
}

bool MaatouchInput::touch_move(int contact, int x, int y, int pressure)
{
    if (!shell_handler_) {
        LogError << "shell handler not ready";
        return false;
    }

    auto [touch_x, touch_y] = screen_to_touch(x, y);

    LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(touch_x) << VAR(touch_y);

    bool ret = shell_handler_->write(MAA_FMT::format("m {} {} {} {}\nc\n", contact, touch_x, touch_y, pressure));
    if (!ret) {
        LogError << "failed to write";
        return false;
    }

    return ret;
}

bool MaatouchInput::touch_up(int contact)
{
    if (!shell_handler_) {
        LogError << "shell handler not ready";
        return false;
    }

    LogInfo << VAR(contact);

    bool ret = shell_handler_->write(MAA_FMT::format("u {}\nc\n", contact));
    if (!ret) {
        LogError << "failed to write";
        return false;
    }

    return ret;
}

MAA_CTRL_UNIT_NS_END
