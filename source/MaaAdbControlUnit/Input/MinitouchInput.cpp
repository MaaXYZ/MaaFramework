#include "MinitouchInput.h"

#include "Utils/Format.hpp"
#include "Utils/Logger.h"
#include "Utils/Ranges.hpp"

#include <array>
#include <cmath>

MAA_CTRL_UNIT_NS_BEGIN

bool MinitouchInput::parse(const json::value& config)
{
    static const json::array kDefaultArch = {
        "x86_64", "x86", "arm64-v8a", "armeabi-v7a", "armeabi",
    };
    json::array jarch = config.get("prebuilt", "minitouch", "arch", kDefaultArch);

    if (MAA_RNS::ranges::any_of(jarch, [](const json::value& val) { return !val.is_string(); })) {
        return false;
    }

    arch_list_ = jarch.to_vector<std::string>();

    return invoke_app_->parse(config);
}

bool MinitouchInput::init(int swidth, int sheight, int orientation)
{
    LogFunc << VAR(swidth) << VAR(sheight) << VAR(orientation);

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

    const auto bin_path = agent_path_ / path(target_arch) / path("minitouch");
    if (!invoke_app_->push(bin_path)) {
        return false;
    }

    if (!invoke_app_->chmod()) {
        return false;
    }

    return set_wh(swidth, sheight, orientation);
}

bool MinitouchInput::set_wh(int swidth, int sheight, int orientation)
{
    LogFunc << VAR(swidth) << VAR(sheight) << VAR(orientation);

    shell_handler_ = invoke_app_->invoke_bin("-i");
    if (!shell_handler_) {
        return false;
    }

    return read_info(swidth, sheight, orientation);
}

MAA_CTRL_UNIT_NS_END
