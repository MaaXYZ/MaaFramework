/*
MIT License

Copyright (c) 2016-2019 Nefarius Software Solutions e.U. and Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// https://github.com/nefarius/ViGEmClient

#pragma once

#include <Windows.h>

namespace vigem
{

// Represents the error codes returned by ViGEm API functions
enum VIGEM_ERROR
{
    VIGEM_ERROR_NONE = 0x20000000,
    VIGEM_ERROR_BUS_NOT_FOUND = 0xE0000001,
    VIGEM_ERROR_NO_FREE_SLOT = 0xE0000002,
    VIGEM_ERROR_INVALID_TARGET = 0xE0000003,
    VIGEM_ERROR_REMOVAL_FAILED = 0xE0000004,
    VIGEM_ERROR_ALREADY_CONNECTED = 0xE0000005,
    VIGEM_ERROR_TARGET_UNINITIALIZED = 0xE0000006,
    VIGEM_ERROR_TARGET_NOT_PLUGGED_IN = 0xE0000007,
    VIGEM_ERROR_BUS_VERSION_MISMATCH = 0xE0000008,
    VIGEM_ERROR_BUS_ACCESS_FAILED = 0xE0000009,
    VIGEM_ERROR_CALLBACK_ALREADY_REGISTERED = 0xE0000010,
    VIGEM_ERROR_CALLBACK_NOT_FOUND = 0xE0000011,
    VIGEM_ERROR_BUS_ALREADY_CONNECTED = 0xE0000012,
    VIGEM_ERROR_BUS_INVALID_HANDLE = 0xE0000013,
    VIGEM_ERROR_XUSB_USERINDEX_OUT_OF_RANGE = 0xE0000014,
    VIGEM_ERROR_INVALID_PARAMETER = 0xE0000015,
    VIGEM_ERROR_NOT_SUPPORTED = 0xE0000016,
    VIGEM_ERROR_WINAPI = 0xE0000017,
    VIGEM_ERROR_TIMED_OUT = 0xE0000018,
    VIGEM_ERROR_IS_DISPOSING = 0xE0000019,
    VIGEM_ERROR_IS_DISPOSED = 0xE000001A,
};

// Checks if a VIGEM_ERROR value indicates success
inline bool VIGEM_SUCCESS(VIGEM_ERROR val)
{
    return val == VIGEM_ERROR_NONE;
}

// Represents the type of virtual gamepad
enum VIGEM_TARGET_TYPE
{
    Xbox360Wired = 0,
    XboxOneWired = 1,
    DualShock4Wired = 2,
};

// Opaque handle to a ViGEm client
using PVIGEM_CLIENT = struct _VIGEM_CLIENT_T*;

// Opaque handle to a virtual gamepad target
using PVIGEM_TARGET = struct _VIGEM_TARGET_T*;

// Xbox 360 controller report structure
struct XUSB_REPORT
{
    USHORT wButtons = 0;
    BYTE bLeftTrigger = 0;
    BYTE bRightTrigger = 0;
    SHORT sThumbLX = 0;
    SHORT sThumbLY = 0;
    SHORT sThumbRX = 0;
    SHORT sThumbRY = 0;
};

// Xbox 360 controller button flags
constexpr USHORT XUSB_GAMEPAD_DPAD_UP = 0x0001;
constexpr USHORT XUSB_GAMEPAD_DPAD_DOWN = 0x0002;
constexpr USHORT XUSB_GAMEPAD_DPAD_LEFT = 0x0004;
constexpr USHORT XUSB_GAMEPAD_DPAD_RIGHT = 0x0008;
constexpr USHORT XUSB_GAMEPAD_START = 0x0010;
constexpr USHORT XUSB_GAMEPAD_BACK = 0x0020;
constexpr USHORT XUSB_GAMEPAD_LEFT_THUMB = 0x0040;
constexpr USHORT XUSB_GAMEPAD_RIGHT_THUMB = 0x0080;
constexpr USHORT XUSB_GAMEPAD_LEFT_SHOULDER = 0x0100;
constexpr USHORT XUSB_GAMEPAD_RIGHT_SHOULDER = 0x0200;
constexpr USHORT XUSB_GAMEPAD_GUIDE = 0x0400;
constexpr USHORT XUSB_GAMEPAD_A = 0x1000;
constexpr USHORT XUSB_GAMEPAD_B = 0x2000;
constexpr USHORT XUSB_GAMEPAD_X = 0x4000;
constexpr USHORT XUSB_GAMEPAD_Y = 0x8000;

// DualShock 4 controller report structure
struct DS4_REPORT
{
    BYTE bThumbLX = 128;
    BYTE bThumbLY = 128;
    BYTE bThumbRX = 128;
    BYTE bThumbRY = 128;
    USHORT wButtons = 0;
    BYTE bSpecial = 0;
    BYTE bTriggerL = 0;
    BYTE bTriggerR = 0;
};

// Function pointer types for dynamic loading
using PFN_vigem_alloc = PVIGEM_CLIENT(WINAPI*)(void);
using PFN_vigem_free = void(WINAPI*)(PVIGEM_CLIENT vigem);
using PFN_vigem_connect = VIGEM_ERROR(WINAPI*)(PVIGEM_CLIENT vigem);
using PFN_vigem_disconnect = void(WINAPI*)(PVIGEM_CLIENT vigem);
using PFN_vigem_target_x360_alloc = PVIGEM_TARGET(WINAPI*)(void);
using PFN_vigem_target_ds4_alloc = PVIGEM_TARGET(WINAPI*)(void);
using PFN_vigem_target_free = void(WINAPI*)(PVIGEM_TARGET target);
using PFN_vigem_target_add = VIGEM_ERROR(WINAPI*)(PVIGEM_CLIENT vigem, PVIGEM_TARGET target);
using PFN_vigem_target_remove = VIGEM_ERROR(WINAPI*)(PVIGEM_CLIENT vigem, PVIGEM_TARGET target);
using PFN_vigem_target_x360_update = VIGEM_ERROR(WINAPI*)(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, XUSB_REPORT report);
using PFN_vigem_target_ds4_update = VIGEM_ERROR(WINAPI*)(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, DS4_REPORT report);

} // namespace vigem
