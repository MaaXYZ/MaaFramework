#pragma once

#if defined(__linux__) && !defined(__ANDROID__)

#include <linux/input-event-codes.h>

#include <array>

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

inline constexpr auto kAsciiToEvdevTable = [] {
    std::array<int, 128> table { };
    // ASCII Letter
#define ASCII_LOWER_UPPER_CASE(ascii, evdev)                                                                                               \
    table[ascii] = evdev;                                                                                                                  \
    table[ascii + 32] = evdev;

    ASCII_LOWER_UPPER_CASE('A', KEY_A)
    ASCII_LOWER_UPPER_CASE('B', KEY_B)
    ASCII_LOWER_UPPER_CASE('C', KEY_C)
    ASCII_LOWER_UPPER_CASE('D', KEY_D)
    ASCII_LOWER_UPPER_CASE('E', KEY_E)
    ASCII_LOWER_UPPER_CASE('F', KEY_F)
    ASCII_LOWER_UPPER_CASE('G', KEY_G)
    ASCII_LOWER_UPPER_CASE('H', KEY_H)
    ASCII_LOWER_UPPER_CASE('I', KEY_I)
    ASCII_LOWER_UPPER_CASE('J', KEY_J)
    ASCII_LOWER_UPPER_CASE('K', KEY_K)
    ASCII_LOWER_UPPER_CASE('L', KEY_L)
    ASCII_LOWER_UPPER_CASE('M', KEY_M)
    ASCII_LOWER_UPPER_CASE('N', KEY_N)
    ASCII_LOWER_UPPER_CASE('O', KEY_O)
    ASCII_LOWER_UPPER_CASE('P', KEY_P)
    ASCII_LOWER_UPPER_CASE('Q', KEY_Q)
    ASCII_LOWER_UPPER_CASE('R', KEY_R)
    ASCII_LOWER_UPPER_CASE('S', KEY_S)
    ASCII_LOWER_UPPER_CASE('T', KEY_T)
    ASCII_LOWER_UPPER_CASE('U', KEY_U)
    ASCII_LOWER_UPPER_CASE('V', KEY_V)
    ASCII_LOWER_UPPER_CASE('W', KEY_W)
    ASCII_LOWER_UPPER_CASE('X', KEY_X)
    ASCII_LOWER_UPPER_CASE('Y', KEY_Y)
    ASCII_LOWER_UPPER_CASE('Z', KEY_Z)

#undef ASCII_LOWER_UPPER_CASE
    // ASCII digit
    table['0'] = KEY_0;
    table['1'] = KEY_1;
    table['2'] = KEY_2;
    table['3'] = KEY_3;
    table['4'] = KEY_4;
    table['5'] = KEY_5;
    table['6'] = KEY_6;
    table['7'] = KEY_7;
    table['8'] = KEY_8;
    table['9'] = KEY_9;
    // Common ASCII / control character mappings
    table['\r'] = KEY_ENTER;
    table['\n'] = KEY_ENTER;
    table['\b'] = KEY_BACKSPACE;
    table['\t'] = KEY_TAB;
    table[27] = KEY_ESC;
    table[' '] = KEY_SPACE;
    table['-'] = KEY_MINUS;
    table['='] = KEY_EQUAL;
    table['['] = KEY_LEFTBRACE;
    table[']'] = KEY_RIGHTBRACE;
    table['\\'] = KEY_BACKSLASH;
    table[';'] = KEY_SEMICOLON;
    table['\''] = KEY_APOSTROPHE;
    table['`'] = KEY_GRAVE;
    table[','] = KEY_COMMA;
    table['.'] = KEY_DOT;
    table['/'] = KEY_SLASH;
    return table;
}();

// Returns the evdev key code for a given ASCII code, or origin value if no mapping exists.
inline int ascii_to_evdev(int ascii)
{
    if (ascii >= 0 && ascii < static_cast<int>(kAsciiToEvdevTable.size())) {
        int mapped = kAsciiToEvdevTable[ascii];
        if (mapped != 0) {
            return mapped;
        }
    }
    return ascii;
}

MAA_CTRL_UNIT_NS_END

#endif
