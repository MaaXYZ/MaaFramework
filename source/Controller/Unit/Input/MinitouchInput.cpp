#include "MinitouchInput.h"

#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"

#include <array>
#include <cmath>
#include <format>

MAA_CTRL_UNIT_NS_BEGIN

bool MinitouchInput::parse(const json::value& config)
{
    return invoke_app_->parse(config);
}

// ordered
static constexpr std::array<std::string_view, 5> kArchList = { "x86_64", "x86", "arm64-v8a", "armeabi-v7a", "armeabi" };

bool MinitouchInput::init(int swidth, int sheight, std::function<std::string(const std::string&)> path_of_bin,
                          const std::string& force_temp)
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return false;
    }

    if (!invoke_app_->init(force_temp)) {
        return false;
    }

    auto archs = invoke_app_->abilist();

    if (!archs) {
        return false;
    }

    auto arch_iter = ranges::find_first_of(*archs, kArchList);
    if (arch_iter == archs->end()) {
        return false;
    }
    const std::string& target_arch = *arch_iter;
    auto bin = path_of_bin(target_arch);

    if (!invoke_app_->push(bin)) {
        return false;
    }

    if (!invoke_app_->chmod()) {
        return false;
    }

    constexpr std::string_view kMinitouchArgs = "-i";
    shell_handler_ = invoke_app_->invoke_bin(std::string(kMinitouchArgs));

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

bool MinitouchInput::click(int x, int y)
{
    if (!shell_handler_) {
        return false;
    }

    if (x < 0 || x >= width_ || y < 0 || y >= height_) {
        LogError << "click point out of range";
        x = std::clamp(x, 0, width_ - 1);
        y = std::clamp(y, 0, height_ - 1);
    }

    scale_point(x, y);

    bool res = shell_handler_->write(std::format("d {} {} {} {}\nc\n", 0, x, y, press_)) &&
               shell_handler_->write(std::format("u\nc\n"));

    if (!res) {
        return false;
    }

    // sleep?
    return true;
}

bool MinitouchInput::swipe(const std::vector<Step>& steps)
{
    if (!shell_handler_ || steps.size() < 2) {
        return false;
    }

    // 检查第一个点的位置?
    {
        auto first = steps[0];
        int x = first.x, y = first.y;
        scale_point(x, y);
        if (!shell_handler_->write(std::format("d {} {} {} {}\nc\n", 0, x, y, press_))) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(first.delay));
    }

    for (auto it = steps.begin() + 1; it != steps.end(); it++) {
        int x = it->x, y = it->y;
        auto res = shell_handler_->write(std::format("m {} {} {} {}\nc\n", 0, x, y, press_));
        if (!res) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(it->delay));
    }

    return shell_handler_->write("u\nc\n");
}

bool MinitouchInput::press_key(int key)
{
    if (!shell_handler_) {
        return false;
    }

    bool res = shell_handler_->write(std::format("k {} d\nc\n", key)) &&
               shell_handler_->write(std::format("k {} u\nc\n", key));

    if (!res) {
        return false;
    }

    // sleep?
    return true;
}

void MinitouchInput::scale_point(int& x, int& y)
{
    x = static_cast<int>(round(x * xscale_));
    y = static_cast<int>(round(y * yscale_));
}

MAA_CTRL_UNIT_NS_END
