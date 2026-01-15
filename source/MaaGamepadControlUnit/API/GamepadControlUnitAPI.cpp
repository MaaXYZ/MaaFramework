#include "ControlUnit/GamepadControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/SafeWindows.hpp"
#include "Manager/GamepadControlUnitMgr.h"

const char* MaaGamepadControlUnitGetVersion()
{
#pragma message("MaaGamepadControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaWin32ControlUnitHandle MaaGamepadControlUnitCreate(void* hWnd, MaaGamepadType gamepad_type, MaaWin32ScreencapMethod screencap_method)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR_VOIDP(hWnd) << VAR(gamepad_type) << VAR(screencap_method);

    HWND h_wnd = reinterpret_cast<HWND>(hWnd);

    auto unit_mgr = std::make_unique<GamepadControlUnitMgr>(h_wnd, gamepad_type, screencap_method);
    return unit_mgr.release();
}

void MaaGamepadControlUnitDestroy(MaaWin32ControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
