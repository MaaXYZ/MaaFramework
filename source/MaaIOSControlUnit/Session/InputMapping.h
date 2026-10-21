#pragma once

#include <algorithm>
#include <optional>
#include <string>

namespace maa::ios_unit
{

// Public controller keys follow Android keycodes, as do MaaFW's other portable units.
inline unsigned short keyboard_usage(int key)
{
    if (key >= 29 && key <= 54) { // KEYCODE_A–Z
        return static_cast<unsigned short>(0x04 + key - 29);
    }
    if (key >= 8 && key <= 16) { // KEYCODE_1–9
        return static_cast<unsigned short>(0x1e + key - 8);
    }
    if (key >= 131 && key <= 142) { // KEYCODE_F1–F12
        return static_cast<unsigned short>(0x3a + key - 131);
    }
    switch (key) {
    case 7:
        return 0x27;                    // KEYCODE_0
    case 19:
        return 0x52;                    // KEYCODE_DPAD_UP
    case 20:
        return 0x51;                    // KEYCODE_DPAD_DOWN
    case 21:
        return 0x50;                    // KEYCODE_DPAD_LEFT
    case 22:
        return 0x4f;                    // KEYCODE_DPAD_RIGHT
    case 55:
        return 0x36;                    // KEYCODE_COMMA
    case 56:
        return 0x37;                    // KEYCODE_PERIOD
    case 57:
        return 0xe2;                    // KEYCODE_ALT_LEFT
    case 58:
        return 0xe6;                    // KEYCODE_ALT_RIGHT
    case 59:
        return 0xe1;                    // KEYCODE_SHIFT_LEFT
    case 60:
        return 0xe5;                    // KEYCODE_SHIFT_RIGHT
    case 61:
        return 0x2b;                    // KEYCODE_TAB
    case 62:
        return 0x2c;                    // KEYCODE_SPACE
    case 66:
        return 0x28;                    // KEYCODE_ENTER
    case 67:
        return 0x2a;                    // KEYCODE_DEL
    case 68:
        return 0x35;                    // KEYCODE_GRAVE
    case 69:
        return 0x2d;                    // KEYCODE_MINUS
    case 70:
        return 0x2e;                    // KEYCODE_EQUALS
    case 71:
        return 0x2f;                    // KEYCODE_LEFT_BRACKET
    case 72:
        return 0x30;                    // KEYCODE_RIGHT_BRACKET
    case 73:
        return 0x31;                    // KEYCODE_BACKSLASH
    case 74:
        return 0x33;                    // KEYCODE_SEMICOLON
    case 75:
        return 0x34;                    // KEYCODE_APOSTROPHE
    case 76:
        return 0x38;                    // KEYCODE_SLASH
    case 92:
        return 0x4b;                    // KEYCODE_PAGE_UP
    case 93:
        return 0x4e;                    // KEYCODE_PAGE_DOWN
    case 111:
        return 0x29;                    // KEYCODE_ESCAPE
    case 112:
        return 0x4c;                    // KEYCODE_FORWARD_DEL
    case 113:
        return 0xe0;                    // KEYCODE_CTRL_LEFT
    case 114:
        return 0xe4;                    // KEYCODE_CTRL_RIGHT
    case 115:
        return 0x39;                    // KEYCODE_CAPS_LOCK
    case 116:
        return 0x47;                    // KEYCODE_SCROLL_LOCK
    case 117:
        return 0xe3;                    // KEYCODE_META_LEFT
    case 118:
        return 0xe7;                    // KEYCODE_META_RIGHT
    case 120:
        return 0x46;                    // KEYCODE_SYSRQ
    case 121:
        return 0x48;                    // KEYCODE_BREAK
    case 122:
        return 0x4a;                    // KEYCODE_MOVE_HOME
    case 123:
        return 0x4d;                    // KEYCODE_MOVE_END
    case 124:
        return 0x49;                    // KEYCODE_INSERT
    case 143:
        return 0x53;                    // KEYCODE_NUM_LOCK
    case 144:
        return 0x62;                    // KEYCODE_NUMPAD_0
    case 154:
        return 0x54;                    // KEYCODE_NUMPAD_DIVIDE
    case 155:
        return 0x55;                    // KEYCODE_NUMPAD_MULTIPLY
    case 156:
        return 0x56;                    // KEYCODE_NUMPAD_SUBTRACT
    case 157:
        return 0x57;                    // KEYCODE_NUMPAD_ADD
    case 158:
        return 0x63;                    // KEYCODE_NUMPAD_DOT
    case 160:
        return 0x58;                    // KEYCODE_NUMPAD_ENTER
    case 161:
        return 0x67;                    // KEYCODE_NUMPAD_EQUALS
    default:
        if (key >= 145 && key <= 153) { // KEYCODE_NUMPAD_1–NUMPAD_9
            return static_cast<unsigned short>(0x59 + key - 145);
        }
        return 0;
    }
}

inline unsigned short consumer_usage(int key)
{
    switch (key) {
    case 3:
        return 0x40; // KEYCODE_HOME
    case 26:
        return 0x30; // KEYCODE_POWER
    case 24:
        return 0xe9; // KEYCODE_VOLUME_UP
    case 25:
        return 0xea; // KEYCODE_VOLUME_DOWN
    case 164:
        return 0xe2; // KEYCODE_VOLUME_MUTE
    default:
        return 0;
    }
}

inline bool ascii_input_supported(const std::string& text)
{
    if (text.empty()) {
        return false;
    }
    return std::all_of(text.begin(), text.end(), [](unsigned char ch) { return (ch >= 0x20 && ch <= 0x7e) || ch == '\n' || ch == '\t'; });
}

struct ImageMapping
{
    int width = 0;
    int height = 0;
    int degrees = 0;
    bool valid = false;

    bool to_panel(int x, int y, int image_width, int image_height, double& px, double& py) const
    {
        const bool swapped = degrees == 90 || degrees == 270;
        if (!valid || width <= 0 || height <= 0 || image_width != (swapped ? height : width)
            || image_height != (swapped ? width : height)) {
            return false;
        }
        const double u = std::clamp(static_cast<double>(x) / image_width, 0.0, 1.0);
        const double v = std::clamp(static_cast<double>(y) / image_height, 0.0, 1.0);
        switch (degrees) {
        case 90:
            px = v;
            py = 1.0 - u;
            break;
        case 180:
            px = 1.0 - u;
            py = 1.0 - v;
            break;
        case 270:
            px = 1.0 - v;
            py = u;
            break;
        default:
            px = u;
            py = v;
            break;
        }
        return true;
    }
};

inline std::optional<int> orientation_degrees(const std::string& value)
{
    if (value == "rot0") {
        return 0;
    }
    if (value == "rot90") {
        return 90;
    }
    if (value == "rot180") {
        return 180;
    }
    if (value == "rot270") {
        return 270;
    }
    return std::nullopt;
}

} // namespace maa::ios_unit
