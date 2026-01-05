#include "ControlUnit/Win32ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/SafeWindows.hpp"
#include "Manager/Win32ControlUnitMgr.h"

const char* MaaWin32ControlUnitGetVersion()
{
#pragma message("MaaWin32ControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaWin32ControlUnitHandle MaaWin32ControlUnitCreate(
    void* hWnd,
    MaaWin32ScreencapMethod screencap_method,
    MaaWin32InputMethod mouse_method,
    MaaWin32InputMethod keyboard_method)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR_VOIDP(hWnd) << VAR(screencap_method) << VAR(mouse_method) << VAR(keyboard_method);

    HWND h_wnd = reinterpret_cast<HWND>(hWnd);

    auto unit_mgr = std::make_unique<Win32ControlUnitMgr>(h_wnd, screencap_method, mouse_method, keyboard_method);
    return unit_mgr.release();
}

void MaaWin32ControlUnitDestroy(MaaWin32ControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
