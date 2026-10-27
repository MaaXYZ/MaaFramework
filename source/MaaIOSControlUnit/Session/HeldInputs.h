#pragma once

#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <utility>

namespace maa::ios_unit
{

struct HeldInputs
{
    std::set<uint16_t> keyboard;
    std::set<uint16_t> buttons;
    bool keyboard_release_pending = false;
    bool touch_active = false;
    std::pair<double, double> last_touch { 0.5, 0.5 };

    using ReleaseTouch = std::function<bool(double, double, std::string&)>;
    using ReleaseKeyboard = std::function<bool(std::string&)>;
    using ReleaseButton = std::function<bool(uint16_t, std::string&)>;

    // Clear only successfully released inputs; a later inactive() can retry failures.
    bool release(const ReleaseTouch& touch, const ReleaseKeyboard& keys, const ReleaseButton& button, std::string& err)
    {
        bool success = true;
        std::string error;
        auto failed = [&] {
            success = false;
            if (err.empty()) {
                err = error;
            }
            error.clear();
        };
        if (touch_active) {
            if (touch(last_touch.first, last_touch.second, error)) {
                touch_active = false;
            }
            else {
                failed();
            }
        }
        if (!keyboard.empty() || keyboard_release_pending) {
            if (keys(error)) {
                keyboard.clear();
                keyboard_release_pending = false;
            }
            else {
                failed();
            }
        }
        for (auto iter = buttons.begin(); iter != buttons.end();) {
            if (button(*iter, error)) {
                iter = buttons.erase(iter);
            }
            else {
                failed();
                ++iter;
            }
        }
        return success;
    }
};

} // namespace maa::ios_unit
