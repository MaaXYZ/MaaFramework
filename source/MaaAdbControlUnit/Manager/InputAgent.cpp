#include "InputAgent.h"

#include <format>
#include <ranges>
#include <unordered_set>

#include "EmulatorExtras/MuMuPlayerExtras.h"
#include "Input/AdbShellInput.h"
#include "Input/MaatouchInput.h"
#include "Input/MinitouchInput.h"
#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

InputAgent::InputAgent(MaaAdbInputMethod methods, const std::filesystem::path& agent_path)
{
    std::vector<Method> method_vector;
    if (methods & MaaAdbInputMethod_EmulatorExtras) {
#ifdef _WIN32
        method_vector.emplace_back(InputAgent::Method::MuMuPlayerExtras);
#else
        LogWarn << "EmulatorExtras is not supported on this platform";
#endif
    }
    if (methods & MaaAdbInputMethod_Maatouch) {
        method_vector.emplace_back(InputAgent::Method::Maatouch);
    }
    if (methods & MaaAdbInputMethod_MinitouchAndAdbKey) {
        method_vector.emplace_back(InputAgent::Method::MinitouchAndAdbKey);
    }
    if (methods & MaaAdbInputMethod_AdbShell) {
        method_vector.emplace_back(InputAgent::Method::AdbShell);
    }

    LogInfo << VAR(methods) << VAR(method_vector) << VAR(agent_path);

    for (Method method : method_vector) {
        std::shared_ptr<InputBase> unit = nullptr;
        switch (method) {
        case Method::AdbShell:
            unit = std::make_shared<AdbShellInput>();
            break;

        case Method::Maatouch: {
            auto maatouch_path = agent_path / "maatouch";
            if (!std::filesystem::exists(maatouch_path)) {
                LogWarn << "maatouch path not exists" << VAR(maatouch_path);
                break;
            }
            unit = std::make_shared<MaatouchInput>(maatouch_path);
        } break;

        case Method::MinitouchAndAdbKey: {
            auto minitouch_path = agent_path / "minitouch";
            if (!std::filesystem::exists(minitouch_path)) {
                LogWarn << "minitouch path not exists" << VAR(minitouch_path);
                break;
            }
            unit = std::make_shared<MinitouchInput>(minitouch_path);
        } break;

#ifdef _WIN32
        case Method::MuMuPlayerExtras:
            unit = std::make_shared<MuMuPlayerExtras>();
            break;
#endif

        default:
            LogWarn << "Not support:" << method;
            break;
        }

        if (!unit) {
            LogWarn << "Create input unit failed:" << method;
            continue;
        }

        children_.emplace_back(unit);
        units_.emplace_back(method, unit);
    }
}

bool InputAgent::parse(const json::value& config)
{
    bool ret = false;
    for (auto& unit : units_ | std::views::values) {
        ret |= unit->parse(config);
    }
    return ret;
}

bool InputAgent::init()
{
    LogFunc;

    if (active_unit_) {
        LogError << "already initialized" << VAR(active_unit_);
        return false;
    }

    for (auto& unit : units_ | std::views::values) {
        if (!unit->init()) {
            continue;
        }
        active_unit_ = std::move(unit);
        break;
    }

    if (!active_unit_) {
        LogError << "No available input method";
        return false;
    }

    units_.clear();
    return true;
}

MaaControllerFeature InputAgent::get_features() const
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return MaaControllerFeature_None;
    }

    return active_unit_->get_features();
}

bool InputAgent::click(int x, int y)
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return false;
    }

    return active_unit_->click(x, y);
}

bool InputAgent::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return false;
    }

    return active_unit_->swipe(x1, y1, x2, y2, duration);
}

bool InputAgent::touch_down(int contact, int x, int y, int pressure)
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return false;
    }

    return active_unit_->touch_down(contact, x, y, pressure);
}

bool InputAgent::touch_move(int contact, int x, int y, int pressure)
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return false;
    }

    return active_unit_->touch_move(contact, x, y, pressure);
}

bool InputAgent::touch_up(int contact)
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return false;
    }

    return active_unit_->touch_up(contact);
}

bool InputAgent::click_key(int key)
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return false;
    }

    return active_unit_->click_key(key);
}

bool InputAgent::input_text(const std::string& text)
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return false;
    }

    return active_unit_->input_text(text);
}

bool InputAgent::key_down(int key)
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return false;
    }

    return active_unit_->key_down(key);
}

bool InputAgent::key_up(int key)
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return false;
    }

    return active_unit_->key_up(key);
}

bool InputAgent::scroll(int dx, int dy)
{
    LogError << "Scroll is not supported on Adb controller" << VAR(dx) << VAR(dy);
    return false;
}

void InputAgent::on_image_resolution_changed(const std::pair<int, int>& pre, const std::pair<int, int>& cur)
{
    if (!active_unit_) {
        LogError << "active_unit_ is null";
        return;
    }

    active_unit_->on_image_resolution_changed(pre, cur);
}

void InputAgent::on_app_started(const std::string& intent)
{
    if (!active_unit_) {
        LogError << "active_unit_ is null";
        return;
    }

    active_unit_->on_app_started(intent);
}

void InputAgent::on_app_stopped(const std::string& intent)
{
    if (!active_unit_) {
        LogError << "active_unit_ is null";
        return;
    }

    active_unit_->on_app_stopped(intent);
}

MAA_CTRL_UNIT_NS_END
