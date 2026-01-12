#include "ControlUnit/GamepadControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "Manager/GamepadControlUnitMgr.h"

const char* MaaGamepadControlUnitGetVersion()
{
#pragma message("MaaGamepadControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaControlUnitHandle MaaGamepadControlUnitCreate(void* hWnd, MaaWin32ScreencapMethod screencap_method)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR_VOIDP(hWnd) << VAR(screencap_method);

    auto unit_mgr = std::make_unique<GamepadControlUnitMgr>(reinterpret_cast<HWND>(hWnd), screencap_method);
    return unit_mgr.release();
}

void MaaGamepadControlUnitDestroy(MaaControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
