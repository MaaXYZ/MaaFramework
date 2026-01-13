#pragma once

#include <cstdint>

namespace vigem
{

// Error codes
inline constexpr uint32_t VIGEM_ERROR_NONE = 0x20000000;

// Xbox button flags
inline constexpr uint16_t XBOX_DPAD_UP = 0x0001;
inline constexpr uint16_t XBOX_DPAD_DOWN = 0x0002;
inline constexpr uint16_t XBOX_DPAD_LEFT = 0x0004;
inline constexpr uint16_t XBOX_DPAD_RIGHT = 0x0008;
inline constexpr uint16_t XBOX_START = 0x0010;
inline constexpr uint16_t XBOX_BACK = 0x0020;
inline constexpr uint16_t XBOX_LEFT_THUMB = 0x0040;
inline constexpr uint16_t XBOX_RIGHT_THUMB = 0x0080;
inline constexpr uint16_t XBOX_LEFT_SHOULDER = 0x0100;
inline constexpr uint16_t XBOX_RIGHT_SHOULDER = 0x0200;
inline constexpr uint16_t XBOX_GUIDE = 0x0400;
inline constexpr uint16_t XBOX_A = 0x1000;
inline constexpr uint16_t XBOX_B = 0x2000;
inline constexpr uint16_t XBOX_X = 0x4000;
inline constexpr uint16_t XBOX_Y = 0x8000;

// DS4 button flags
inline constexpr uint16_t DS4_SQUARE = 0x0010;
inline constexpr uint16_t DS4_CROSS = 0x0020;
inline constexpr uint16_t DS4_CIRCLE = 0x0040;
inline constexpr uint16_t DS4_TRIANGLE = 0x0080;
inline constexpr uint16_t DS4_L1 = 0x0100;
inline constexpr uint16_t DS4_R1 = 0x0200;
inline constexpr uint16_t DS4_L2 = 0x0400;
inline constexpr uint16_t DS4_R2 = 0x0800;
inline constexpr uint16_t DS4_SHARE = 0x1000;
inline constexpr uint16_t DS4_OPTIONS = 0x2000;
inline constexpr uint16_t DS4_L_THUMB = 0x4000;
inline constexpr uint16_t DS4_R_THUMB = 0x8000;

// DS4 special buttons
inline constexpr uint8_t DS4_SPECIAL_PS = 0x01;
inline constexpr uint8_t DS4_SPECIAL_TOUCHPAD = 0x02;

// DS4 DPAD values
inline constexpr uint16_t DS4_DPAD_NONE = 0x8;

#pragma pack(push, 1)

struct XusbReport
{
    uint16_t wButtons = 0;
    uint8_t bLeftTrigger = 0;
    uint8_t bRightTrigger = 0;
    int16_t sThumbLX = 0;
    int16_t sThumbLY = 0;
    int16_t sThumbRX = 0;
    int16_t sThumbRY = 0;
};

struct Ds4Report
{
    uint8_t bThumbLX = 0x80;
    uint8_t bThumbLY = 0x80;
    uint8_t bThumbRX = 0x80;
    uint8_t bThumbRY = 0x80;
    uint16_t wButtons = DS4_DPAD_NONE;
    uint8_t bSpecial = 0;
    uint8_t bTriggerL = 0;
    uint8_t bTriggerR = 0;
};

#pragma pack(pop)

// ViGEm function pointer types
using FnVigemAlloc = void* (*)();
using FnVigemFree = void (*)(void*);
using FnVigemConnect = uint32_t (*)(void*);
using FnVigemDisconnect = void (*)(void*);
using FnVigemTargetAlloc = void* (*)();
using FnVigemTargetFree = void (*)(void*);
using FnVigemTargetAdd = uint32_t (*)(void*, void*);
using FnVigemTargetRemove = uint32_t (*)(void*, void*);
using FnVigemTargetX360Update = uint32_t (*)(void*, void*, XusbReport);
using FnVigemTargetDs4Update = uint32_t (*)(void*, void*, Ds4Report);

} // namespace vigem
