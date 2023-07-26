#include "MaatouchInput.h"

#include "Utils/Format.hpp"
#include "Utils/Logger.hpp"
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
        auto opt = mopt->find<json::value>("root");
        if (!opt) {
            LogError << "Cannot find entry prebuilt.maatouch.root";
            return false;
        }

        if (!opt->is_string()) {
            return false;
        }

        root_ = opt->as_string();
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

bool MaatouchInput::init(int swidth, int sheight)
{
    LogFunc;

    if (!invoke_app_->init()) {
        return false;
    }

    auto bin = MAA_FMT::format("{}/universal/maatouch", root_);

    if (!invoke_app_->push(bin)) {
        return false;
    }

    if (!invoke_app_->chmod()) {
        return false;
    }

    shell_handler_ = invoke_app_->invoke_app(package_name_);

    if (!shell_handler_) {
        return false;
    }

    // TODO: timeout?
    std::string prev {};
    while (true) {
        auto str = prev + shell_handler_->read(5);
        if (str.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
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
        auto info = str.substr(pos + 1, rpos - pos - 1);
        LogInfo << "minitouch info:" << info;

        int contact = 0;
        int x = 0;
        int y = 0;
        int pressure = 0;

        std::istringstream ins(info);
        if (!ins >> contact >> x >> y >> pressure) {
            return false;
        }

        width_ = swidth;
        height_ = sheight;
        xscale_ = double(x) / swidth;
        yscale_ = double(y) / sheight;
        press_ = pressure;

        return true;
    }
}

bool MaatouchInput::click(int x, int y)
{
    if (!shell_handler_) {
        LogError << "shell handler not ready";
        return false;
    }

    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        LogError << "click point out of range";
        x = std::clamp(x, 0, width_ - 1);
        y = std::clamp(y, 0, height_ - 1);
    }

    auto [real_x, real_y] = scale_point(x, y);

    LogInfo << VAR(x) << VAR(y) << VAR(real_x) << VAR(real_y);

    bool res = shell_handler_->write(MAA_FMT::format("d {} {} {} {}\nc\n", 0, real_x, real_y, press_)) &&
               shell_handler_->write(MAA_FMT::format("u {}\nc\n", 0));

    if (!res) {
        LogError << "click failed";
        return false;
    }

    // sleep?
    return true;
}

bool MaatouchInput::swipe(const std::vector<SwipeStep>& steps)
{
    if (!shell_handler_ || steps.size() < 2) {
        return false;
    }

    // 检查第一个点的位置?
    {
        auto first = steps[0];
        int x = first.x, y = first.y;
        scale_point(x, y);
        auto now = std::chrono::steady_clock::now();
        if (!shell_handler_->write(MAA_FMT::format("d {} {} {} {}\nc\n", 0, x, y, press_))) {
            return false;
        }
        auto used = std::chrono::steady_clock::now() - now;
        auto expect = std::chrono::milliseconds(first.delay);
        if (used < expect) {
            std::this_thread::sleep_for(expect - used);
        }
    }

    for (auto it = steps.begin() + 1; it != steps.end(); it++) {
        int x = it->x, y = it->y;
        auto now = std::chrono::steady_clock::now();
        auto res = shell_handler_->write(MAA_FMT::format("m {} {} {} {}\nc\n", 0, x, y, press_));
        if (!res) {
            return false;
        }
        auto used = std::chrono::steady_clock::now() - now;
        auto expect = std::chrono::milliseconds(it->delay);
        if (used < expect) {
            std::this_thread::sleep_for(expect - used);
        }
    }

    return shell_handler_->write(MAA_FMT::format("u {}\nc\n", 0));
}

bool MaatouchInput::press_key(int key)
{
    if (!shell_handler_) {
        return false;
    }

    bool res = shell_handler_->write(MAA_FMT::format("k {} d\nc\n", key)) &&
               shell_handler_->write(MAA_FMT::format("k {} u\nc\n", key));

    if (!res) {
        return false;
    }

    // sleep?
    return true;
}

std::pair<int, int> MaatouchInput::scale_point(int x, int y)
{
    return std::make_pair(static_cast<int>(round(x * xscale_)), static_cast<int>(round(y * yscale_)));
}

MAA_CTRL_UNIT_NS_END
