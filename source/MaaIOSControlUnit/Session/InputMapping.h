#pragma once

#include <algorithm>
#include <optional>
#include <string>

namespace maa::ios_unit
{

// Public controller keys follow Android keycodes, as do MaaFW's other portable units.
inline unsigned short keyboard_usage(int key)
{
    if (key >= 29 && key <= 54) {
        return static_cast<unsigned short>(0x04 + key - 29);
    }
    if (key >= 8 && key <= 16) {
        return static_cast<unsigned short>(0x1e + key - 8);
    }
    if (key >= 131 && key <= 142) {
        return static_cast<unsigned short>(0x3a + key - 131);
    }
    switch (key) {
    case 7:
        return 0x27; // 0
    case 19:
        return 0x52;
    case 20:
        return 0x51;
    case 21:
        return 0x50;
    case 22:
        return 0x4f;
    case 55:
        return 0x36;
    case 56:
        return 0x37;
    case 57:
        return 0xe2;
    case 58:
        return 0xe6;
    case 59:
        return 0xe1;
    case 60:
        return 0xe5;
    case 61:
        return 0x2b;
    case 62:
        return 0x2c;
    case 66:
        return 0x28;
    case 67:
        return 0x2a;
    case 68:
        return 0x35;
    case 69:
        return 0x2d;
    case 70:
        return 0x2e;
    case 71:
        return 0x2f;
    case 72:
        return 0x30;
    case 73:
        return 0x31;
    case 74:
        return 0x33;
    case 75:
        return 0x34;
    case 76:
        return 0x38;
    case 92:
        return 0x4b;
    case 93:
        return 0x4e;
    case 111:
        return 0x29;
    case 112:
        return 0x4c;
    case 113:
        return 0xe0;
    case 114:
        return 0xe4;
    case 115:
        return 0x39;
    case 116:
        return 0x47;
    case 117:
        return 0xe3;
    case 118:
        return 0xe7;
    case 120:
        return 0x46;
    case 121:
        return 0x48;
    case 122:
        return 0x4a;
    case 123:
        return 0x4d;
    case 124:
        return 0x49;
    case 143:
        return 0x53;
    case 144:
        return 0x62;
    case 154:
        return 0x54;
    case 155:
        return 0x55;
    case 156:
        return 0x56;
    case 157:
        return 0x57;
    case 158:
        return 0x63;
    case 160:
        return 0x58;
    case 161:
        return 0x67;
    default:
        if (key >= 145 && key <= 153) {
            return static_cast<unsigned short>(0x59 + key - 145);
        }
        return 0;
    }
}

inline unsigned short consumer_usage(int key)
{
    switch (key) {
    case 3:
        return 0x40; // home
    case 26:
        return 0x30; // lock
    case 24:
        return 0xe9;
    case 25:
        return 0xea;
    case 164:
        return 0xe2;
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
