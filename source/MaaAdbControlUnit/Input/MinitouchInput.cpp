#include "MinitouchInput.h"

#include <array>
#include <cmath>
#include <format>
#include <ranges>

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

MinitouchInput::~MinitouchInput()
{
    remove_binary();
}

bool MinitouchInput::parse(const json::value& config)
{
    static const json::array kDefaultArch = {
        "x86_64", "x86", "arm64-v8a", "armeabi-v7a", "armeabi",
    };
    json::array jarch = config.get("prebuilt", "minitouch", "arch", kDefaultArch);

    if (!jarch.all<std::string>()) {
        return false;
    }

    arch_list_ = jarch.as<std::vector<std::string>>();

    return invoke_app_->parse(config) && MtouchHelper::parse(config) && adb_shell_input_->parse(config);
}

bool MinitouchInput::init()
{
    LogFunc;

    if (!invoke_app_->init()) {
        return false;
    }

    auto archs = invoke_app_->abilist();

    if (!archs) {
        return false;
    }

    auto arch_iter = std::ranges::find_first_of(*archs, arch_list_);
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

    return invoke_and_read_info();
}

MaaControllerFeature MinitouchInput::get_features() const
{
    MaaControllerFeature feat = MaaControllerFeature_UseMouseDownAndUpInsteadOfClick;
    if (adb_shell_input_) {
        feat |= adb_shell_input_->get_features() & MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
    }
    return feat;
}

bool MinitouchInput::click_key(int key)
{
    if (!adb_shell_input_) {
        LogError << "adb_shell_input_ is nullptr";
        return false;
    }

    return adb_shell_input_->click_key(key);
}

bool MinitouchInput::input_text(const std::string& text)
{
    if (!adb_shell_input_) {
        LogError << "adb_shell_input_ is nullptr";
        return false;
    }

    return adb_shell_input_->input_text(text);
}

bool MinitouchInput::key_down(int key)
{
    if (!adb_shell_input_) {
        LogError << "adb_shell_input_ is nullptr";
        return false;
    }

    return adb_shell_input_->key_down(key);
}

bool MinitouchInput::key_up(int key)
{
    if (!adb_shell_input_) {
        LogError << "adb_shell_input_ is nullptr";
        return false;
    }

    return adb_shell_input_->key_up(key);
}

void MinitouchInput::on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur)
{
    std::ignore = pre;
    std::ignore = cur;

    invoke_and_read_info();
}

bool MinitouchInput::invoke_and_read_info()
{
    // https://github.com/openstf/minitouch#running
    static const std::string kMinitouchUseStdin = "-i";
    pipe_ios_ = invoke_app_->invoke_bin(kMinitouchUseStdin);
    if (!pipe_ios_) {
        return false;
    }

    return read_info();
}

void MinitouchInput::remove_binary()
{
    LogTrace;

    invoke_app_->remove();
}

MAA_CTRL_UNIT_NS_END
