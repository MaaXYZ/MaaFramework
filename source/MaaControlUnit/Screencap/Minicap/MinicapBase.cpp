#include "MinicapBase.h"

#include "Utils/Logger.hpp"
#include "Utils/Format.hpp"
#include "Utils/NoWarningCV.h"
#include "Utils/Ranges.hpp"

#include <array>

MAA_CTRL_UNIT_NS_BEGIN

bool MinicapBase::parse(const json::value& config)
{
    auto popt = config.find<json::object>("prebuilt");
    if (!popt) {
        LogError << "Cannot find entry prebuilt";
        return false;
    }

    auto mopt = popt->find<json::object>("minicap");
    if (!mopt) {
        LogError << "Cannot find entry prebuilt.minicap";
        return false;
    }

    {
        auto opt = mopt->find<json::value>("root");
        if (!opt) {
            LogError << "Cannot find entry prebuilt.minicap.root";
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
            LogError << "Cannot find entry prebuilt.minicap.arch";
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
        MAA_RNS::ranges::transform(arr, std::back_inserter(arch_list_), [](const json::value& val) { return val.as_string(); });
    }

    {
        auto opt = mopt->find<json::value>("sdk");
        if (!opt) {
            LogError << "Cannot find entry prebuilt.minicap.sdk";
            return false;
        }

        const auto& value = *opt;
        if (!value.is_array()) {
            return false;
        }

        const auto& arr = value.as_array();
        if (MAA_RNS::ranges::any_of(arr, [](const json::value& val) { return !val.is_number(); })) {
            return false;
        }

        sdk_list_.clear();
        sdk_list_.reserve(arr.size());
        MAA_RNS::ranges::transform(arr, std::back_inserter(sdk_list_), [](const json::value& val) { return val.as_integer(); });
    }

    return binary_->parse(config) && library_->parse(config);
}

// x86_64的prebuilt里面的library是32位的, 用不了
// arm64-v8会卡住, 不知道原因
bool MinicapBase::init(int w, int h)
{
    LogFunc;

    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return false;
    }

    if (!binary_->init() || !library_->init("minicap.so")) {
        return false;
    }

    auto archs = binary_->abilist();
    auto sdk = binary_->sdk();

    if (!archs || !sdk) {
        return false;
    }

    auto arch_iter = MAA_RNS::ranges::find_first_of(*archs, arch_list_);
    if (arch_iter == archs->end()) {
        return false;
    }
    const std::string& target_arch = *arch_iter;

    auto sdk_iter = MAA_RNS::ranges::find_if(sdk_list_, [sdk](int s) { return s <= sdk.value(); });
    if (sdk_iter == sdk_list_.end()) {
        return false;
    }
    int fit_sdk = *sdk_iter;

    // TODO: 确认低版本是否使用minicap-nopie
    auto bin = MAA_FMT::format("{}/{}/bin/minicap", root_, target_arch);
    auto lib = MAA_FMT::format("{}/{}/lib/android-{}/minicap.so", root_, target_arch, fit_sdk);

    if (!binary_->push(bin) || !library_->push(lib)) {
        return false;
    }

    if (!binary_->chmod() || !library_->chmod()) {
        return false;
    }

    screencap_helper_.set_wh(w, h);
    return true;
}

MAA_CTRL_UNIT_NS_END
