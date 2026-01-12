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

#pragma once

#include <Windows.h>

#include "Common.h"

#ifdef VIGEM_DYNAMIC
#ifdef VIGEM_EXPORTS
#define VIGEM_API __declspec(dllexport)
#else
#define VIGEM_API __declspec(dllimport)
#endif
#else
#define VIGEM_API
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    //
    // Represents a driver connection object.
    //
    typedef struct _VIGEM_CLIENT_T* PVIGEM_CLIENT;

    //
    // Represents an emulated (virtual) gamepad object.
    //
    typedef struct _VIGEM_TARGET_T* PVIGEM_TARGET;

    //
    // Error codes returned by the ViGEm API
    //
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
        VIGEM_ERROR_TIMED_OUT = 0xE0000018

    } VIGEM_ERROR;

//
// Checks if a given return value indicates a successfully completed request.
//
#define VIGEM_SUCCESS(val) (val == VIGEM_ERROR_NONE)

    typedef VOID(CALLBACK* PFN_VIGEM_X360_NOTIFICATION)(
        PVIGEM_CLIENT Client,
        PVIGEM_TARGET Target,
        UCHAR LargeMotor,
        UCHAR SmallMotor,
        UCHAR LedNumber,
        LPVOID UserData);

    typedef VOID(CALLBACK* PFN_VIGEM_DS4_NOTIFICATION)(
        PVIGEM_CLIENT Client,
        PVIGEM_TARGET Target,
        UCHAR LargeMotor,
        UCHAR SmallMotor,
        DS4_LIGHTBAR_COLOR LightbarColor,
        LPVOID UserData);

    //
    // Allocates an object representing a driver connection.
    //
    VIGEM_API PVIGEM_CLIENT vigem_alloc(void);

    //
    // Frees up memory used by the driver connection object.
    //
    VIGEM_API void vigem_free(PVIGEM_CLIENT vigem);

    //
    // Initializes the driver object and establishes a connection to the emulation bus driver.
    //
    VIGEM_API VIGEM_ERROR vigem_connect(PVIGEM_CLIENT vigem);

    //
    // Disconnects from the bus driver and resets the driver object state.
    //
    VIGEM_API void vigem_disconnect(PVIGEM_CLIENT vigem);

    //
    // Allocates an object representing an Xbox 360 Controller device.
    //
    VIGEM_API PVIGEM_TARGET vigem_target_x360_alloc(void);

    //
    // Allocates an object representing a DualShock 4 Controller device.
    //
    VIGEM_API PVIGEM_TARGET vigem_target_ds4_alloc(void);

    //
    // Frees up memory used by the target device object.
    //
    VIGEM_API void vigem_target_free(PVIGEM_TARGET target);

    //
    // Adds a provided target device to the bus driver.
    //
    VIGEM_API VIGEM_ERROR vigem_target_add(PVIGEM_CLIENT vigem, PVIGEM_TARGET target);

    //
    // Removes a provided target device from the bus driver.
    //
    VIGEM_API VIGEM_ERROR vigem_target_remove(PVIGEM_CLIENT vigem, PVIGEM_TARGET target);

    //
    // Sends a state report to the provided Xbox 360 target device.
    //
    VIGEM_API VIGEM_ERROR vigem_target_x360_update(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, XUSB_REPORT report);

    //
    // Sends a state report to the provided DualShock 4 target device.
    //
    VIGEM_API VIGEM_ERROR vigem_target_ds4_update(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, DS4_REPORT report);

    //
    // Sends a state report to the provided DualShock 4 target device and receives output report.
    //
    VIGEM_API VIGEM_ERROR
        vigem_target_ds4_update_ex(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, DS4_REPORT report, PDS4_OUTPUT_BUFFER output);

    //
    // Returns the internal index of the target device.
    //
    VIGEM_API ULONG vigem_target_get_index(PVIGEM_TARGET target);

    //
    // Returns the type of the target device.
    //
    VIGEM_API VIGEM_TARGET_TYPE vigem_target_get_type(PVIGEM_TARGET target);

    //
    // Returns TRUE if the target device is currently attached to the bus.
    //
    VIGEM_API BOOL vigem_target_is_attached(PVIGEM_TARGET target);

    //
    // Returns the user index of the emulated Xbox 360 device.
    //
    VIGEM_API VIGEM_ERROR vigem_target_x360_get_user_index(PVIGEM_CLIENT vigem, PVIGEM_TARGET target, PULONG index);

    //
    // Registers a function which gets called upon Xbox 360 controller output report arrival.
    //
    VIGEM_API VIGEM_ERROR vigem_target_x360_register_notification(
        PVIGEM_CLIENT vigem,
        PVIGEM_TARGET target,
        PFN_VIGEM_X360_NOTIFICATION notification,
        LPVOID userData);

    //
    // Removes a previously registered callback function.
    //
    VIGEM_API void vigem_target_x360_unregister_notification(PVIGEM_TARGET target);

    //
    // Registers a function which gets called upon DualShock 4 controller output report arrival.
    //
    VIGEM_API VIGEM_ERROR vigem_target_ds4_register_notification(
        PVIGEM_CLIENT vigem,
        PVIGEM_TARGET target,
        PFN_VIGEM_DS4_NOTIFICATION notification,
        LPVOID userData);

    //
    // Removes a previously registered callback function.
    //
    VIGEM_API void vigem_target_ds4_unregister_notification(PVIGEM_TARGET target);

    //
    // Sets the serial number of the provided target device.
    //
    VIGEM_API void vigem_target_set_vid(PVIGEM_TARGET target, USHORT vid);

    //
    // Sets the product ID of the provided target device.
    //
    VIGEM_API void vigem_target_set_pid(PVIGEM_TARGET target, USHORT pid);

    //
    // Returns the Vendor ID of the provided target device.
    //
    VIGEM_API USHORT vigem_target_get_vid(PVIGEM_TARGET target);

    //
    // Returns the Product ID of the provided target device.
    //
    VIGEM_API USHORT vigem_target_get_pid(PVIGEM_TARGET target);

#ifdef __cplusplus
}
#endif
