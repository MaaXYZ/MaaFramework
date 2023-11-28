#include "MaatouchInput.h"

#include "Utils/Format.hpp"
#include "Utils/Logger.h"
#include "Utils/Ranges.hpp"

#include <array>
#include <cmath>

MAA_CTRL_UNIT_NS_BEGIN

bool MaatouchInput::parse(const json::value& config)
{
    package_name_ = config.get("prebuilt", "maatouch", "package", "com.shxyke.MaaTouch.App");

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

    // https://github.com/openstf/minitouch#writable-to-the-socket
    static constexpr std::string_view kKeyDownFormat = "k {} d\nc\n";
    static constexpr std::string_view kKeyUpFormat = "k {} u\nc\n";

    bool ret = shell_handler_->write(MAA_FMT::format(kKeyDownFormat, key)) &&
               shell_handler_->write(MAA_FMT::format(kKeyUpFormat, key));

    if (!ret) {
        LogError << "failed to write";
        return false;
    }

    return ret;
}

MAA_CTRL_UNIT_NS_END
