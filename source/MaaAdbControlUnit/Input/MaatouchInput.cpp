#include "MaatouchInput.h"

#include <array>
#include <cmath>
#include <format>
#include <ranges>

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

MaatouchInput::~MaatouchInput()
{
    remove_binary();
}

bool MaatouchInput::parse(const json::value& config)
{
    static const std::string kDefaultPackage = "com.shxyke.MaaTouch.App";
    package_name_ = config.get("prebuilt", "maatouch", "package", kDefaultPackage);

    return invoke_app_->parse(config) && MtouchHelper::parse(config);
}

bool MaatouchInput::init()
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

    return invoke_and_read_info();
}

MaaControllerFeature MaatouchInput::get_features() const
{
    return MaaControllerFeature_UseMouseDownAndUpInsteadOfClick | MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
}

bool MaatouchInput::click_key(int key)
{
    LogError << "deprecated" << VAR(key);
    return false;
}

bool MaatouchInput::input_text(const std::string& text)
{
    LogInfo << VAR(text);

    if (!pipe_ios_) {
        LogError << "pipe_ios_ is nullptr";
        return false;
    }

    // https://github.com/MaaAssistantArknights/MaaTouch
    static constexpr std::string_view kTextFormat = "t {}\nc\n";

    bool ret = pipe_ios_->write(std::format(kTextFormat, text));

    if (!ret) {
        LogError << "failed to write";
        return false;
    }

    return true;
}

bool MaatouchInput::key_down(int key)
{
    LogInfo << VAR(key);

    if (!pipe_ios_) {
        LogError << "pipe_ios_ is nullptr";
        return false;
    }

    // https://github.com/openstf/minitouch#writable-to-the-socket
    static constexpr std::string_view kKeyDownFormat = "k {} d\nc\n";

    bool ret = pipe_ios_->write(std::format(kKeyDownFormat, key));

    if (!ret) {
        LogError << "failed to write";
        return false;
    }

    return true;
}

bool MaatouchInput::key_up(int key)
{
    LogInfo << VAR(key);

    if (!pipe_ios_) {
        LogError << "pipe_ios_ is nullptr";
        return false;
    }

    // https://github.com/openstf/minitouch#writable-to-the-socket
    static constexpr std::string_view kKeyUpFormat = "k {} u\nc\n";

    bool ret = pipe_ios_->write(std::format(kKeyUpFormat, key));

    if (!ret) {
        LogError << "failed to write";
        return false;
    }

    return true;
}

void MaatouchInput::on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur)
{
    std::ignore = pre;
    std::ignore = cur;

    invoke_and_read_info();
}

bool MaatouchInput::invoke_and_read_info()
{
    pipe_ios_ = invoke_app_->invoke_app(package_name_);
    if (!pipe_ios_) {
        return false;
    }

    return read_info();
}

void MaatouchInput::remove_binary()
{
    LogTrace;

    invoke_app_->remove();
}

MAA_CTRL_UNIT_NS_END
