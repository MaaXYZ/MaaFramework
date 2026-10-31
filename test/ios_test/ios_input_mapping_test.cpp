#include "Session/HeldInputs.h"
#include "Session/InputMapping.h"

#include <cmath>
#include <iostream>

int main()
{
    using namespace maa::ios_unit;
    auto require = [](bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            return false;
        }
        return true;
    };
    if (!require(keyboard_usage(45) == 0x14 && keyboard_usage(29) == 0x04, "Android alphabet keycode mapping")) {
        return 1;
    }
    if (!require(keyboard_usage(7) == 0x27 && keyboard_usage(16) == 0x26, "Android number keycode mapping")) {
        return 1;
    }
    if (!require(keyboard_usage(113) == 0xe0 && keyboard_usage(118) == 0xe7, "left/right modifiers")) {
        return 1;
    }
    if (!require(keyboard_usage(4) == 0 && consumer_usage(4) == 0, "unsupported Android back must fail")) {
        return 1;
    }
    if (!require(consumer_usage(24) == 0xe9 && consumer_usage(26) == 0x30, "hardware button mapping")) {
        return 1;
    }
    if (!require(
            ascii_input_supported("hello\tworld\n!123") && !ascii_input_supported("hello中文") && !ascii_input_supported("")
                && !ascii_input_supported(std::string("a\0b", 3)),
            "reject whole unsupported string before input")) {
        return 1;
    }
    double x = 0, y = 0;
    ImageMapping portrait { 1125, 2436, 0, true };
    if (!require(portrait.to_panel(562, 1218, 1125, 2436, x, y) && std::abs(y - 0.5) < 1e-9, "portrait pixel normalization")) {
        return 1;
    }
    ImageMapping clockwise { 1125, 2436, 90, true };
    if (!require(clockwise.to_panel(609, 562, 2436, 1125, x, y) && std::abs(y - 0.75) < 1e-9, "landscape inverse rotation")) {
        return 1;
    }
    // The saved screenshot geometry remains authoritative even if later metadata reports portrait.
    portrait.degrees = 180;
    if (!require(clockwise.to_panel(609, 562, 2436, 1125, x, y) && std::abs(y - 0.75) < 1e-9, "saved geometry independence")) {
        return 1;
    }
    if (!require(!clockwise.to_panel(1, 1, 1125, 2436, x, y), "mismatched image geometry refused")) {
        return 1;
    }
    clockwise.valid = false;
    if (!require(!clockwise.to_panel(1, 1, 2436, 1125, x, y), "unknown screenshot geometry refused")) {
        return 1;
    }
    if (!require(!orientation_degrees("unknown").has_value() && orientation_degrees("rot270") == 270, "do not guess orientation")) {
        return 1;
    }
    HeldInputs held;
    held.keyboard = { 0xe1, 0x04 };
    held.buttons = { 0xe9, 0x30 };
    held.touches = { { 0, { 0.2, 0.7 } }, { 3, { 0.8, 0.3 } } };
    std::string error;
    int keys_released = 0, touch_released = 0, buttons_released = 0;
    const bool released = held.release(
        [&](int contact, double px, double py, std::string& err) {
            ++touch_released;
            if (contact == 3) {
                err = "touch release failed";
                return false;
            }
            return contact == 0 && px == 0.2 && py == 0.7;
        },
        [&](std::string&) {
            ++keys_released;
            return true;
        },
        [&](uint16_t usage, std::string& err) {
            if (usage == 0xe9) {
                err = "volume release failed";
                return false;
            }
            ++buttons_released;
            return true;
        },
        error);
    if (!require(
            !released && keys_released == 1 && touch_released == 2 && buttons_released == 1 && held.keyboard.empty() && held.touches.size() == 1 && held.touches.contains(3)
                && held.buttons == std::set<uint16_t> { 0xe9 } && error == "touch release failed",
            "cleanup releases all inputs and retains only failures")) {
        return 1;
    }
    error.clear();
    held.keyboard_release_pending = true; // A failed text sequence may leave an unknown key held.
    if (!require(
            held.release(
                [](int contact, double px, double py, std::string&) { return contact == 3 && px == 0.8 && py == 0.3; },
                [&](std::string&) {
                    ++keys_released;
                    return true;
                },
                [](uint16_t, std::string&) { return true; },
                error)
                && keys_released == 2 && held.buttons.empty() && held.touches.empty() && !held.keyboard_release_pending,
            "retry failed input and release partial text")) {
        return 1;
    }
    return 0;
}
