#include "MtouchHelper.h"

#include <array>
#include <cmath>
#include <format>
#include <ranges>

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN

std::string MtouchHelper::type_name() const
{
    return typeid(this).name();
}

std::pair<int, int> MtouchHelper::screen_to_touch(int x, int y)
{
    return screen_to_touch_impl(x, y);
}

std::pair<int, int> MtouchHelper::screen_to_touch(double x, double y)
{
    return screen_to_touch_impl(x, y);
}

MaaControllerFeature MtouchHelper::get_features() const
{
    MaaControllerFeature feat = MaaControllerFeature_UseMouseDownAndUpInsteadOfClick;
    if (adb_shell_input_) {
        feat |= adb_shell_input_->get_features() & MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
    }
    return feat;
}

bool MtouchHelper::click_key(int key)
{
    if (!adb_shell_input_) {
        LogError << "adb_shell_input_ is nullptr";
        return false;
    }
    return adb_shell_input_->click_key(key);
}

bool MtouchHelper::input_text(const std::string& text)
{
    if (!adb_shell_input_) {
        LogError << "adb_shell_input_ is nullptr";
        return false;
    }
    return adb_shell_input_->input_text(text);
}

bool MtouchHelper::key_down(int key)
{
    if (!adb_shell_input_) {
        LogError << "adb_shell_input_ is nullptr";
        return false;
    }
    return adb_shell_input_->key_down(key);
}

bool MtouchHelper::key_up(int key)
{
    if (!adb_shell_input_) {
        LogError << "adb_shell_input_ is nullptr";
        return false;
    }
    return adb_shell_input_->key_up(key);
}

bool MtouchHelper::parse_minitouch_config(const json::value& config)
{
    static const json::array kDefaultArch = {
        "x86_64", "x86", "arm64-v8a", "armeabi-v7a", "armeabi",
    };
    json::array jarch = config.get("prebuilt", "minitouch", "arch", kDefaultArch);

    if (!jarch.all<std::string>()) {
        return false;
    }

    arch_list_ = jarch.as<std::vector<std::string>>();

    return invoke_app_->parse(config) && parse(config) && adb_shell_input_->parse(config);
}

bool MtouchHelper::push_minitouch()
{
    if (!invoke_app_->init()) {
        return false;
    }

    auto archs = invoke_app_->abilist();
    if (!archs) {
        return false;
    }

    auto arch_iter = std::ranges::find_first_of(*archs, arch_list_);
    if (arch_iter == archs->end()) {
        LogError << "no matching arch for minitouch" << VAR(*archs) << VAR(arch_list_);
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

    return true;
}

void MtouchHelper::remove_binary()
{
    LogTrace;
    if (invoke_app_) {
        invoke_app_->remove();
    }
}

bool MtouchHelper::read_info()
{
    if (!pipe_ios_) {
        LogError << type_name() << "pipe_ios_ is nullptr";
        return false;
    }

    using namespace std::chrono_literals;
    std::ignore = pipe_ios_->read_until("^", 5s);
    constexpr std::string_view kFlag = "\n";
    std::string info = pipe_ios_->read_until(kFlag, 1s);

    if (!info.ends_with(kFlag)) {
        LogError << type_name() << "failed to read info";
        return false;
    }
    LogInfo << type_name() << VAR(info);

    int contact = 0;
    int x = 0;
    int y = 0;
    int pressure = 0;

    std::istringstream ins(std::move(info));
    if (!(ins >> contact >> x >> y >> pressure)) {
        return false;
    }

    if (!request_display_info()) {
        LogWarn << type_name() << "failed to request display info";
        return false;
    }

    bool landscape = display_width_ > display_height_;
    touch_width_ = landscape ? std::max(x, y) : std::min(x, y);
    touch_height_ = landscape ? std::min(x, y) : std::max(x, y);
    xscale_ = static_cast<double>(touch_width_) / display_width_;
    yscale_ = static_cast<double>(touch_height_) / display_height_;
    press_ = pressure;

    LogInfo << type_name() << VAR(display_width_) << VAR(display_height_) << VAR(touch_width_) << VAR(touch_height_) << VAR(xscale_)
            << VAR(yscale_) << VAR(press_) << VAR(orientation_);

    return true;
}

bool MtouchHelper::request_display_info()
{
    auto resolution_opt = device_info_->request_resolution();
    if (!resolution_opt) {
        LogError << type_name() << "failed to request resolution";
        return false;
    }

    display_width_ = resolution_opt->w;
    display_height_ = resolution_opt->h;
    orientation_ = resolution_opt->r;

    auto orientation_opt = device_info_->request_orientation();
    if (!orientation_opt) {
        LogWarn << type_name() << "failed to request orientation";
        return true;
    }

    orientation_ = *orientation_opt;

    return true;
}

// get_features() 返回 MaaControllerFeature_UseMouseDownAndUpInsteadOfClick，
// 上层 ControllerAgent 会使用 touch_down/touch_up 替代 click/swipe
bool MtouchHelper::click(int x, int y)
{
    LogError << type_name()
             << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_up instead"
             << VAR(x) << VAR(y);

    return false;
}

bool MtouchHelper::swipe(int x1, int y1, int x2, int y2, int duration)
{
    LogError << type_name()
             << "deprecated: get_features() returns MaaControllerFeature_UseMouseDownAndUpInsteadOfClick, "
                "use touch_down/touch_move/touch_up instead"
             << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    return false;
}

bool MtouchHelper::touch_down(int contact, int x, int y, int pressure)
{
    if (!pipe_ios_) {
        LogError << type_name() << "pipe_ios_ is nullptr";
        return false;
    }

    auto [touch_x, touch_y] = screen_to_touch(x, y);

    LogInfo << type_name() << VAR(contact) << VAR(x) << VAR(y) << VAR(touch_x) << VAR(touch_y);

    bool ret = pipe_ios_->write(std::format(kDownFormat, contact, touch_x, touch_y, pressure));

    if (!ret) {
        LogError << type_name() << "failed to write";
        return false;
    }

    return true;
}

bool MtouchHelper::touch_move(int contact, int x, int y, int pressure)
{
    if (!pipe_ios_) {
        LogError << type_name() << "pipe_ios_ is nullptr";
        return false;
    }

    auto [touch_x, touch_y] = screen_to_touch(x, y);

    // LogInfo << VAR(contact) << VAR(x) << VAR(y) << VAR(touch_x) << VAR(touch_y);

    bool ret = pipe_ios_->write(std::format(kMoveFormat, contact, touch_x, touch_y, pressure));

    if (!ret) {
        LogError << type_name() << "failed to write";
        return false;
    }

    return true;
}

bool MtouchHelper::touch_up(int contact)
{
    if (!pipe_ios_) {
        LogError << "pipe_ios_ is nullptr";
        return false;
    }

    LogInfo << type_name() << VAR(contact);

    bool ret = pipe_ios_->write(std::format(kUpFormat, contact));

    if (!ret) {
        LogError << type_name() << "failed to write";
        return false;
    }

    return true;
}

bool MtouchHelper::parse(const json::value& config)
{
    return device_info_->parse(config);
}

MAA_CTRL_UNIT_NS_END
