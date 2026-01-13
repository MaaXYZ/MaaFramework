#pragma once

#include <memory>
#include <optional>

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaGamepadControlUnitGetVersion();

    /**
     * @brief Create a virtual gamepad control unit.
     *
     * @param hWnd Window handle for screencap (optional, can be nullptr if screencap not needed).
     * @param gamepad_type Type of virtual gamepad (MaaGamepadType_Xbox360 or MaaGamepadType_DualShock4).
     * @param screencap_method Win32 screencap method to use (MaaWin32ScreencapMethod_*).
     *                         Ignored if hWnd is nullptr.
     * @return The control unit handle, or nullptr on failure.
     *
     * @note Requires ViGEm Bus Driver to be installed on the system.
     * @note For gamepad control, use:
     *       - click_key/key_down/key_up: For digital buttons (A, B, X, Y, LB, RB, etc.)
     *       - touch_down/touch_move/touch_up: For analog inputs (sticks and triggers)
     *         - contact 0: Left stick (x: -32768~32767, y: -32768~32767)
     *         - contact 1: Right stick (x: -32768~32767, y: -32768~32767)
     *         - contact 2: Left trigger (pressure: 0~255, x/y ignored)
     *         - contact 3: Right trigger (pressure: 0~255, x/y ignored)
     * @see MaaGamepadButton, MaaGamepadTouch, MaaGamepadType
     */
    MAA_CONTROL_UNIT_API MaaWin32ControlUnitHandle MaaGamepadControlUnitCreate(
        void* hWnd,
        MaaGamepadType gamepad_type,
        MaaWin32ScreencapMethod screencap_method);

    MAA_CONTROL_UNIT_API void MaaGamepadControlUnitDestroy(MaaWin32ControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
