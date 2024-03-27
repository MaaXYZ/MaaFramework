#include "MaatouchInput.h"

#include <array>
#include <cmath>
#include <format>
#include <ranges>

#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

bool MaatouchInput::parse(const json::value& config)
{
    static const std::string kDefaultPackage = "com.shxyke.MaaTouch.App";
    package_name_ = config.get("prebuilt", "maatouch", "package", kDefaultPackage);

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

    pipe_ios_ = invoke_app_->invoke_app(package_name_);
    if (!pipe_ios_) {
        return false;
    }

    return read_info(swidth, sheight, orientation);
}

bool MaatouchInput::init()
{
    return touch_width_ > 0 && touch_height_ > 0;
}

bool MaatouchInput::press_key(int key)
{
    LogInfo << VAR(key);

    if (!pipe_ios_) {
        LogError << "pipe_ios_ is nullptr";
        return false;
    }

    // https://github.com/openstf/minitouch#writable-to-the-socket
    static constexpr std::string_view kKeyDownFormat = "k {} d\nc\n";
    static constexpr std::string_view kKeyUpFormat = "k {} u\nc\n";

    bool ret = pipe_ios_->write(std::format(kKeyDownFormat, key))
               && pipe_ios_->write(std::format(kKeyUpFormat, key));

    if (!ret) {
        LogError << "failed to write";
        return false;
    }

    return true;
}

bool MaatouchInput::input_text(const std::string& text)
{
    LogInfo << VAR(text);

    if (std::ranges::any_of(text, [](char ch) { return static_cast<unsigned char>(ch) > 127; })) {
        LogError << "maatouch key input does not support non-ascii text, skip" << VAR(text);
        return false;
    }

    bool ret = true;
    for (char ch : text) {
        ret &= press_key(ch);
    }
    return ret;
}

MAA_CTRL_UNIT_NS_END