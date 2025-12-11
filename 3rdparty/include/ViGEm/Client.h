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

#ifdef __cplusplus
extern "C"
{
#endif

    // Represents the error codes returned by ViGEm API functions
    typedef enum _VIGEM_ERROR
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
    } VIGEM_ERROR;

// Checks if a VIGEM_ERROR value indicates success
#define VIGEM_SUCCESS(val) (val == VIGEM_ERROR_NONE)

    // Represents the type of virtual gamepad
    typedef enum _VIGEM_TARGET_TYPE
    {
        Xbox360Wired = 0,
        XboxOneWired = 1,
        DualShock4Wired = 2,
    } VIGEM_TARGET_TYPE;

    // Opaque handle to a ViGEm client
    typedef struct _VIGEM_CLIENT_T* PVIGEM_CLIENT;

    // Opaque handle to a virtual gamepad target
    typedef struct _VIGEM_TARGET_T* PVIGEM_TARGET;

    // Xbox 360 controller report structure
    typedef struct _XUSB_REPORT
    {
        USHORT wButtons;
        BYTE bLeftTrigger;
        BYTE bRightTrigger;
        SHORT sThumbLX;
        SHORT sThumbLY;
        SHORT sThumbRX;
        SHORT sThumbRY;
    } XUSB_REPORT, *PXUSB_REPORT;

// Initialize XUSB_REPORT to zero
#define XUSB_REPORT_INIT(report)     \
    do {                             \
        (report)->wButtons = 0;      \
        (report)->bLeftTrigger = 0;  \
        (report)->bRightTrigger = 0; \
        (report)->sThumbLX = 0;      \
        (report)->sThumbLY = 0;      \
        (report)->sThumbRX = 0;      \
        (report)->sThumbRY = 0;      \
    } while (0)

// Xbox 360 controller button flags
#define XUSB_GAMEPAD_DPAD_UP 0x0001
#define XUSB_GAMEPAD_DPAD_DOWN 0x0002
#define XUSB_GAMEPAD_DPAD_LEFT 0x0004
#define XUSB_GAMEPAD_DPAD_RIGHT 0x0008
#define XUSB_GAMEPAD_START 0x0010
#define XUSB_GAMEPAD_BACK 0x0020
#define XUSB_GAMEPAD_LEFT_THUMB 0x0040
#define XUSB_GAMEPAD_RIGHT_THUMB 0x0080
#define XUSB_GAMEPAD_LEFT_SHOULDER 0x0100
#define XUSB_GAMEPAD_RIGHT_SHOULDER 0x0200
#define XUSB_GAMEPAD_GUIDE 0x0400
#define XUSB_GAMEPAD_A 0x1000
#define XUSB_GAMEPAD_B 0x2000
#define XUSB_GAMEPAD_X 0x4000
#define XUSB_GAMEPAD_Y 0x8000

    // DualShock 4 controller report structure
    typedef struct _DS4_REPORT
    {
        BYTE bThumbLX;
        BYTE bThumbLY;
        BYTE bThumbRX;
        BYTE bThumbRY;
        USHORT wButtons;
        BYTE bSpecial;
        BYTE bTriggerL;
        BYTE bTriggerR;
    } DS4_REPORT, *PDS4_REPORT;

#define DS4_REPORT_INIT(report)   \
    do {                          \
        (report)->bThumbLX = 128; \
        (report)->bThumbLY = 128; \
        (report)->bThumbRX = 128; \
        (report)->bThumbRY = 128; \
        (report)->wButtons = 0;   \
        (report)->bSpecial = 0;   \
        (report)->bTriggerL = 0;  \
        (report)->bTriggerR = 0;  \
    } while (0)

    // Function pointer types for dynamic loading
    typedef PVIGEM_CLIENT(WINAPI* PFN_vigem_alloc)(void);
    typedef void(WINAPI* PFN_vigem_free)(PVIGEM_CLIENT vigem);
    typedef VIGEM_ERROR(WINAPI* PFN_vigem_connect)(PVIGEM_CLIENT vigem);
    typedef void(WINAPI* PFN_vigem_disconnect)(PVIGEM_CLIENT vigem);
    typedef PVIGEM_TARGET(WINAPI* PFN_vigem_target_x360_alloc)(void);
    typedef PVIGEM_TARGET(WINAPI* PFN_vigem_target_ds4_alloc)(void);
    typedef void(WINAPI* PFN_vigem_target_free)(PVIGEM_TARGET target);
    typedef VIGEM_ERROR(WINAPI* PFN_vigem_target_add)(PVIGEM_CLIENT vigem, PVIGEM_TARGET target);
    typedef VIGEM_ERROR(WINAPI* PFN_vigem_target_remove)(PVIGEM_CLIENT vigem, PVIGEM_TARGET target);
    typedef VIGEM_ERROR(WINAPI* PFN_vigem_target_x360_update)(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, XUSB_REPORT report);
    typedef VIGEM_ERROR(WINAPI* PFN_vigem_target_ds4_update)(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, DS4_REPORT report);

#ifdef __cplusplus
}
#endif
