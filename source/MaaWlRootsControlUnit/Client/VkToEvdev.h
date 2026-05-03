#pragma once

#include <linux/input-event-codes.h>

#include <array>

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

// Win32 Virtual-Key (VK_*) code to Linux evdev KEY_* code.
// VK code is the index (0x00-0xFF); the value is the corresponding evdev key code,
// or 0 when no reasonable mapping exists.
//
// VK_SHIFT / VK_CONTROL / VK_MENU default to the left variant, while the explicit
// L/R codes (0xA0..0xA5) map to their respective left/right keys.
// OEM punctuation keys use the US layout.
//
// References:
//   - VK codes: https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
//   - evdev codes: linux/input-event-codes.h
inline constexpr auto kVkToEvdevTable = [] {
    std::array<int, 256> table { };

    // 0x08..0x14 control keys
    table[0x08] = KEY_BACKSPACE; // VK_BACK
    table[0x09] = KEY_TAB;       // VK_TAB
    table[0x0C] = KEY_CLEAR;     // VK_CLEAR
    table[0x0D] = KEY_ENTER;     // VK_RETURN
    table[0x10] = KEY_LEFTSHIFT; // VK_SHIFT
    table[0x11] = KEY_LEFTCTRL;  // VK_CONTROL
    table[0x12] = KEY_LEFTALT;   // VK_MENU
    table[0x13] = KEY_PAUSE;     // VK_PAUSE
    table[0x14] = KEY_CAPSLOCK;  // VK_CAPITAL
    table[0x1B] = KEY_ESC;       // VK_ESCAPE

    // 0x20..0x2E navigation
    table[0x20] = KEY_SPACE;    // VK_SPACE
    table[0x21] = KEY_PAGEUP;   // VK_PRIOR
    table[0x22] = KEY_PAGEDOWN; // VK_NEXT
    table[0x23] = KEY_END;
    table[0x24] = KEY_HOME;
    table[0x25] = KEY_LEFT;
    table[0x26] = KEY_UP;
    table[0x27] = KEY_RIGHT;
    table[0x28] = KEY_DOWN;
    table[0x2C] = KEY_SYSRQ;  // VK_SNAPSHOT (Print Screen)
    table[0x2D] = KEY_INSERT; // VK_INSERT
    table[0x2E] = KEY_DELETE;

    // 0x30..0x39 '0'..'9'
    table[0x30] = KEY_0;
    table[0x31] = KEY_1;
    table[0x32] = KEY_2;
    table[0x33] = KEY_3;
    table[0x34] = KEY_4;
    table[0x35] = KEY_5;
    table[0x36] = KEY_6;
    table[0x37] = KEY_7;
    table[0x38] = KEY_8;
    table[0x39] = KEY_9;

    // 0x41..0x5A 'A'..'Z'
    table[0x41] = KEY_A;
    table[0x42] = KEY_B;
    table[0x43] = KEY_C;
    table[0x44] = KEY_D;
    table[0x45] = KEY_E;
    table[0x46] = KEY_F;
    table[0x47] = KEY_G;
    table[0x48] = KEY_H;
    table[0x49] = KEY_I;
    table[0x4A] = KEY_J;
    table[0x4B] = KEY_K;
    table[0x4C] = KEY_L;
    table[0x4D] = KEY_M;
    table[0x4E] = KEY_N;
    table[0x4F] = KEY_O;
    table[0x50] = KEY_P;
    table[0x51] = KEY_Q;
    table[0x52] = KEY_R;
    table[0x53] = KEY_S;
    table[0x54] = KEY_T;
    table[0x55] = KEY_U;
    table[0x56] = KEY_V;
    table[0x57] = KEY_W;
    table[0x58] = KEY_X;
    table[0x59] = KEY_Y;
    table[0x5A] = KEY_Z;

    // 0x5B..0x5D VK_LWIN / VK_RWIN / VK_APPS
    table[0x5B] = KEY_LEFTMETA;
    table[0x5C] = KEY_RIGHTMETA;
    table[0x5D] = KEY_COMPOSE;

    // 0x60..0x6F numpad
    table[0x60] = KEY_KP0;
    table[0x61] = KEY_KP1;
    table[0x62] = KEY_KP2;
    table[0x63] = KEY_KP3;
    table[0x64] = KEY_KP4;
    table[0x65] = KEY_KP5;
    table[0x66] = KEY_KP6;
    table[0x67] = KEY_KP7;
    table[0x68] = KEY_KP8;
    table[0x69] = KEY_KP9;
    table[0x6A] = KEY_KPASTERISK; // VK_MULTIPLY
    table[0x6B] = KEY_KPPLUS;     // VK_ADD
    table[0x6C] = KEY_KPENTER;    // VK_SEPARATOR
    table[0x6D] = KEY_KPMINUS;    // VK_SUBTRACT
    table[0x6E] = KEY_KPDOT;      // VK_DECIMAL
    table[0x6F] = KEY_KPSLASH;    // VK_DIVIDE

    // 0x70..0x87 VK_F1..VK_F24
    table[0x70] = KEY_F1;
    table[0x71] = KEY_F2;
    table[0x72] = KEY_F3;
    table[0x73] = KEY_F4;
    table[0x74] = KEY_F5;
    table[0x75] = KEY_F6;
    table[0x76] = KEY_F7;
    table[0x77] = KEY_F8;
    table[0x78] = KEY_F9;
    table[0x79] = KEY_F10;
    table[0x7A] = KEY_F11;
    table[0x7B] = KEY_F12;
    table[0x7C] = KEY_F13;
    table[0x7D] = KEY_F14;
    table[0x7E] = KEY_F15;
    table[0x7F] = KEY_F16;
    table[0x80] = KEY_F17;
    table[0x81] = KEY_F18;
    table[0x82] = KEY_F19;
    table[0x83] = KEY_F20;
    table[0x84] = KEY_F21;
    table[0x85] = KEY_F22;
    table[0x86] = KEY_F23;
    table[0x87] = KEY_F24;

    // 0x90..0x91 lock keys
    table[0x90] = KEY_NUMLOCK;
    table[0x91] = KEY_SCROLLLOCK;

    // 0xA0..0xA5 explicit L/R modifier variants
    table[0xA0] = KEY_LEFTSHIFT;
    table[0xA1] = KEY_RIGHTSHIFT;
    table[0xA2] = KEY_LEFTCTRL;
    table[0xA3] = KEY_RIGHTCTRL;
    table[0xA4] = KEY_LEFTALT;
    table[0xA5] = KEY_RIGHTALT;

    // 0xA6..0xAC browser keys
    table[0xA6] = KEY_BACK;
    table[0xA7] = KEY_FORWARD;
    table[0xA8] = KEY_REFRESH;
    table[0xA9] = KEY_STOP;
    table[0xAA] = KEY_SEARCH;
    table[0xAB] = KEY_BOOKMARKS;
    table[0xAC] = KEY_HOMEPAGE;

    // 0xAD..0xAF volume
    table[0xAD] = KEY_MUTE;
    table[0xAE] = KEY_VOLUMEDOWN;
    table[0xAF] = KEY_VOLUMEUP;

    // 0xB0..0xB3 media transport
    table[0xB0] = KEY_NEXTSONG;
    table[0xB1] = KEY_PREVIOUSSONG;
    table[0xB2] = KEY_STOPCD;
    table[0xB3] = KEY_PLAYPAUSE;

    // 0xB4..0xB7 launch keys
    table[0xB4] = KEY_MAIL;  // VK_LAUNCH_MAIL
    table[0xB5] = KEY_MEDIA; // VK_LAUNCH_MEDIA_SELECT
    table[0xB6] = KEY_PROG1; // VK_LAUNCH_APP1
    table[0xB7] = KEY_PROG2; // VK_LAUNCH_APP2

    // 0xBA..0xC0, 0xDB..0xDE OEM punctuation (US layout)
    table[0xBA] = KEY_SEMICOLON;  // ';:'
    table[0xBB] = KEY_EQUAL;      // '=+'
    table[0xBC] = KEY_COMMA;      // ',<'
    table[0xBD] = KEY_MINUS;      // '-_'
    table[0xBE] = KEY_DOT;        // '.>'
    table[0xBF] = KEY_SLASH;      // '/?'
    table[0xC0] = KEY_GRAVE;      // '`~'
    table[0xDB] = KEY_LEFTBRACE;  // '[{'
    table[0xDC] = KEY_BACKSLASH;  // '\|'
    table[0xDD] = KEY_RIGHTBRACE; // ']}'
    table[0xDE] = KEY_APOSTROPHE; // '\'"'

    return table;
}();

// Returns the evdev key code for a given Win32 VK code, or 0 if no mapping exists.
inline int vk_to_evdev(int vk)
{
    if (vk < 0 || vk >= static_cast<int>(kVkToEvdevTable.size())) {
        return 0;
    }
    return kVkToEvdevTable[vk];
}

MAA_CTRL_UNIT_NS_END
