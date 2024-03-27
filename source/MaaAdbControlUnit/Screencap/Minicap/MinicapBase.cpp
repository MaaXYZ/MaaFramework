#include "MinicapBase.h"

#include <array>
#include <format>
#include <ranges>

#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

bool MinicapBase::parse(const json::value& config)
{
    static const json::array kDefaultArch = {
        "x86",
        "armeabi-v7a",
        "armeabi",
    };
    json::array jarch = config.get("prebuilt", "minicap", "arch", kDefaultArch);

    if (!jarch.all<std::string>()) {
        return false;
    }
    arch_list_ = jarch.as_collection<std::string>();

    static const json::array kDefaultSdk = {
        31, 29, 28, 27, 26, 25, 24, 23, 22, 21, 19, 18, 17, 16, 15, 14,
    };
    json::array jsdk = config.get("prebuilt", "minicap", "sdk", kDefaultSdk);
    sdk_list_ = jsdk.as_collection<int>();

    return binary_->parse(config) && library_->parse(config);
}

// x86_64的prebuilt里面的library是32位的, 用不了
// arm64-v8会卡住, 不知道原因
bool MinicapBase::init(int swidth, int sheight)
{
    LogFunc;

    if (!binary_->init() || !library_->init("minicap.so")) {
        return false;
    }

    auto archs_opt = binary_->abilist();
    auto sdk_opt = binary_->sdk();

    if (!archs_opt || !sdk_opt) {
        return false;
    }

    auto arch_iter = std::ranges::find_first_of(*archs_opt, arch_list_);
    if (arch_iter == archs_opt->end()) {
        return false;
    }
    const std::string& target_arch = *arch_iter;

    auto sdk_iter =
        std::ranges::find_if(sdk_list_, [sdk_opt](int s) { return s <= sdk_opt.value(); });
    if (sdk_iter == sdk_list_.end()) {
        return false;
    }
    int fit_sdk = *sdk_iter;

    // TODO: 确认低版本是否使用minicap-nopie
    const auto bin_path = agent_path_ / path(target_arch) / path("bin") / path("minicap");
    const auto lib_path = agent_path_ / path(target_arch) / path("lib")
                          / path(std::format("android-{}", fit_sdk)) / path("minicap.so");
    if (!binary_->push(bin_path) || !library_->push(lib_path)) {
        return false;
    }

    if (!binary_->chmod() || !library_->chmod()) {
        return false;
    }

    return set_wh(swidth, sheight);
}

MAA_CTRL_UNIT_NS_END
