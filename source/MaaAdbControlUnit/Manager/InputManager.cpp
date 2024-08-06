#include "InputManager.h"

#include <format>
#include <ranges>
#include <unordered_set>

#include "EmulatorExtras/MumuExternalRendererIpc.h"
#include "Input/AdbShellInput.h"
#include "Input/MaatouchInput.h"
#include "Input/MinitouchInput.h"
#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

InputManager::InputManager(const MethodVec& input_methods, const std::filesystem::path& agent_path)
{
    LogInfo << VAR(input_methods) << VAR(agent_path);

    for (Method method : input_methods) {
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
        case Method::MumuExternalRendererIpc:
            unit = std::make_shared<MumuExternalRendererIpc>();
            break;
        default:
            LogWarn << "Not support:" << method;
            break;
        }

        children_.emplace_back(unit);
        units_.emplace_back(method, unit);
    }
}

bool InputManager::parse(const json::value& config)
{
    bool ret = false;
    for (auto& unit : units_ | std::views::values) {
        ret |= unit->parse(config);
    }
    return ret;
}

bool InputManager::init()
{
    LogFunc;

    for (auto& unit : units_ | std::views::values) {
        if (!unit->init()) {
            continue;
        }
        active_unit_ = unit;
        break;
    }

    return active_unit_ != nullptr;
}

void InputManager::deinit()
{
    for (auto& unit : units_ | std::views::values) {
        unit->deinit();
    }
}

bool InputManager::click(int x, int y)
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return false;
    }

    return active_unit_->click(x, y);
}

bool InputManager::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return false;
    }

    return active_unit_->swipe(x1, y1, x2, y2, duration);
}

bool InputManager::touch_down(int contact, int x, int y, int pressure)
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return false;
    }

    return active_unit_->touch_down(contact, x, y, pressure);
}

bool InputManager::touch_move(int contact, int x, int y, int pressure)
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return false;
    }

    return active_unit_->touch_move(contact, x, y, pressure);
}

bool InputManager::touch_up(int contact)
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return false;
    }

    return active_unit_->touch_up(contact);
}

bool InputManager::press_key(int key)
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return false;
    }

    return active_unit_->press_key(key);
}

bool InputManager::input_text(const std::string& text)
{
    if (!active_unit_) {
        LogError << "No available input method" << VAR(active_unit_);
        return false;
    }

    return active_unit_->input_text(text);
}

std::ostream& operator<<(std::ostream& os, InputManager::Method m)
{
    switch (m) {
    case InputManager::Method::UnknownYet:
        os << "UnknownYet";
        break;
    case InputManager::Method::AdbShell:
        os << "AdbShell";
        break;
    case InputManager::Method::MinitouchAndAdbKey:
        os << "MinitouchAndAdbKey";
        break;
    case InputManager::Method::Maatouch:
        os << "Maatouch";
        break;
    case InputManager::Method::MumuExternalRendererIpc:
        os << "MumuExternalRendererIpc";
        break;
    default:
        os << "Unknown";
        break;
    }
    return os;
}

MAA_CTRL_UNIT_NS_END
