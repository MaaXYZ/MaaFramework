#include "AutoDetectInput.h"

#include <format>
#include <ranges>
#include <unordered_set>

#include "AdbInput.h"
#include "MaatouchInput.h"
#include "MinitouchInput.h"
#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

AutoDetectInput::AutoDetectInput(
    const std::filesystem::path& maatouch_path,
    const std::filesystem::path& minitouch_path)
{
    if (std::filesystem::exists(maatouch_path)) {
        auto maatouch = std::make_shared<MaatouchInput>(maatouch_path);

        touch_units_.emplace_back(std::make_pair(TouchMethod::Maatouch, maatouch));

        key_units_.emplace_back(std::make_pair(KeyMethod::Maatouch, maatouch));
    }
    else {
        LogWarn << "maatouch_path path not exists" << VAR(maatouch_path);
    }

    if (std::filesystem::exists(minitouch_path)) {
        touch_units_.emplace_back(std::make_pair(
            TouchMethod::Minitouch,
            std::make_shared<MinitouchInput>(minitouch_path)));
    }
    else {
        LogWarn << "minitouch_path path not exists" << VAR(minitouch_path);
    }

    touch_units_.emplace_back(std::make_pair(TouchMethod::AdbTap, std::make_shared<AdbTapInput>()));
    key_units_.emplace_back(std::make_pair(KeyMethod::AdbKey, std::make_shared<AdbKeyInput>()));

    for (auto& unit : touch_units_ | std::views::values) {
        children_.emplace_back(unit);
    }
    for (auto& unit : key_units_ | std::views::values) {
        children_.emplace_back(unit);
    }
}

bool AutoDetectInput::parse(const json::value& config)
{
    bool ret = false;
    for (auto& unit : touch_units_ | std::views::values) {
        ret |= unit->parse(config);
    }
    for (auto& unit : key_units_ | std::views::values) {
        ret |= unit->parse(config);
    }
    return ret;
}

bool AutoDetectInput::init(int swidth, int sheight, int orientation)
{
    LogFunc;

    for (auto& unit : touch_units_ | std::views::values) {
        if (!unit->init(swidth, sheight, orientation)) {
            continue;
        }
        available_touch_ = unit;
        break;
    }

    return available_touch_ != nullptr;
}

void AutoDetectInput::deinit()
{
    for (auto& unit : touch_units_ | std::views::values) {
        unit->deinit();
    }
    for (auto& unit : key_units_ | std::views::values) {
        unit->deinit();
    }
}

bool AutoDetectInput::set_wh(int swidth, int sheight, int orientation)
{
    bool ret = false;
    for (auto& unit : touch_units_ | std::views::values) {
        ret |= unit->set_wh(swidth, sheight, orientation);
    }
    return ret;
}

bool AutoDetectInput::click(int x, int y)
{
    if (!available_touch_) {
        LogError << "No available touch method" << VAR(available_touch_);
        return false;
    }

    return available_touch_->click(x, y);
}

bool AutoDetectInput::swipe(int x1, int y1, int x2, int y2, int duration)
{
    if (!available_touch_) {
        LogError << "No available touch method" << VAR(available_touch_);
        return false;
    }

    return available_touch_->swipe(x1, y1, x2, y2, duration);
}

bool AutoDetectInput::touch_down(int contact, int x, int y, int pressure)
{
    if (!available_touch_) {
        LogError << "No available touch method" << VAR(available_touch_);
        return false;
    }

    return available_touch_->touch_down(contact, x, y, pressure);
}

bool AutoDetectInput::touch_move(int contact, int x, int y, int pressure)
{
    if (!available_touch_) {
        LogError << "No available touch method" << VAR(available_touch_);
        return false;
    }

    return available_touch_->touch_move(contact, x, y, pressure);
}

bool AutoDetectInput::touch_up(int contact)
{
    if (!available_touch_) {
        LogError << "No available touch method" << VAR(available_touch_);
        return false;
    }

    return available_touch_->touch_up(contact);
}

bool AutoDetectInput::init()
{
    for (auto& unit : key_units_ | std::views::values) {
        if (!unit->init()) {
            continue;
        }
        available_key_ = unit;
        break;
    }

    return available_key_ != nullptr;
}

bool AutoDetectInput::press_key(int key)
{
    if (!available_key_) {
        LogError << "No available key method" << VAR(available_key_);
        return false;
    }

    return available_key_->press_key(key);
}

bool AutoDetectInput::input_text(const std::string& text)
{
    if (!available_key_) {
        LogError << "No available key method" << VAR(available_key_);
        return false;
    }

    return available_key_->input_text(text);
}

MAA_CTRL_UNIT_NS_END