#include "MinitouchInput.h"

#include "Utils/Format.hpp"
#include "Utils/Logger.h"
#include "Utils/Ranges.hpp"

#include <array>
#include <cmath>

MAA_CTRL_UNIT_NS_BEGIN

bool MinitouchInput::parse(const json::value& config)
{
    auto popt = config.find<json::object>("prebuilt");
    if (!popt) {
        LogError << "Cannot find entry prebuilt";
        return false;
    }

    auto mopt = popt->find<json::object>("minitouch");
    if (!mopt) {
        LogError << "Cannot find entry prebuilt.minitouch";
        return false;
    }

    {
        auto opt = mopt->find<json::value>("root");
        if (!opt) {
            LogError << "Cannot find entry prebuilt.minitouch.root";
            return false;
        }

        if (!opt->is_string()) {
            return false;
        }

        root_ = opt->as_string();
    }

    {
        auto opt = mopt->find<json::value>("arch");
        if (!opt) {
            LogError << "Cannot find entry prebuilt.minitouch.arch";
            return false;
        }

        const auto& value = *opt;
        if (!value.is_array()) {
            return false;
        }

        const auto& arr = value.as_array();
        if (MAA_RNS::ranges::any_of(arr, [](const json::value& val) { return !val.is_string(); })) {
            return false;
        }

        arch_list_.clear();
        arch_list_.reserve(arr.size());
        MAA_RNS::ranges::transform(arr, std::back_inserter(arch_list_),
                                   [](const json::value& val) { return val.as_string(); });
    }

    return invoke_app_->parse(config);
}

bool MinitouchInput::init(int swidth, int sheight, int orientation)
{
    LogFunc;

    if (!invoke_app_->init()) {
        return false;
    }

    auto archs = invoke_app_->abilist();

    if (!archs) {
        return false;
    }

    auto arch_iter = MAA_RNS::ranges::find_first_of(*archs, arch_list_);
    if (arch_iter == archs->end()) {
        return false;
    }
    const std::string& target_arch = *arch_iter;
    auto bin = MAA_FMT::format("{}/{}/minitouch", root_, target_arch);

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
    std::string prev;
    std::string info;
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

        info = str.substr(pos + 1, rpos - pos - 1);
        break;
    }

    LogInfo << "minitouch info:" << info;
    string_trim_(info);

    int contact = 0;
    int x = 0;
    int y = 0;
    int pressure = 0;

    std::istringstream ins(std::move(info));
    if (!(ins >> contact >> x >> y >> pressure)) {
        return false;
    }

    swidth_ = swidth;
    sheight_ = sheight;
    twidth_ = x;
    theight_ = y;
    xscale_ = double(x) / swidth;
    yscale_ = double(y) / sheight;
    press_ = pressure;
    orientation_ = orientation;

    return true;
}

void MinitouchInput::set_wh(int swidth, int sheight, int orientation)
{
    init(swidth, sheight, orientation);
}

bool MinitouchInput::click(int x, int y)
{
    if (!shell_handler_) {
        LogError << "shell handler not ready";
        return false;
    }

    if (x < 0 || x >= swidth_ || y < 0 || y >= sheight_) {
        LogError << "click point out of range";
        x = std::clamp(x, 0, swidth_ - 1);
        y = std::clamp(y, 0, sheight_ - 1);
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

bool MinitouchInput::swipe(const std::vector<SwipeStep>& steps)
{
    if (!shell_handler_ || steps.size() < 2) {
        return false;
    }

    // 检查第一个点的位置?
    {
        auto first = steps[0];
        auto [x, y] = scale_point(first.x, first.y);
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

std::pair<int, int> MinitouchInput::scale_point(int x, int y)
{
    auto int_round = [](double v) -> int { return static_cast<int>(round(v)); };

    switch (orientation_) {
    case 0:
    default:
        return {
            int_round(x * xscale_),
            int_round(y * yscale_),
        };
    case 1:
        return {
            theight_ - int_round(y * yscale_),
            int_round(x * xscale_),
        };
    case 2:
        return {
            twidth_ - int_round(x * xscale_),
            theight_ - int_round(y * yscale_),
        };
    case 3:
        return {
            int_round(y * yscale_),
            twidth_ - int_round(x * xscale_),
        };
    }
}

MAA_CTRL_UNIT_NS_END
