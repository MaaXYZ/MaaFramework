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
    LogFunc << VAR(swidth) << VAR(sheight) << VAR(orientation);

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

    return set_wh(swidth, sheight, orientation);
}

bool MaatouchInput::set_wh(int swidth, int sheight, int orientation)
{
    LogFunc << VAR(swidth) << VAR(sheight) << VAR(orientation);

    shell_handler_ = invoke_app_->invoke_app(package_name_);
    if (!shell_handler_) {
        return false;
    }

    return read_info(swidth, sheight, orientation);
}

bool MaatouchInput::press_key(int key)
{
    LogInfo << VAR(key);

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

MAA_CTRL_UNIT_NS_END
