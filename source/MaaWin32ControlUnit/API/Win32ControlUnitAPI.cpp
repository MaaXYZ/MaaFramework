#include "ControlUnit/Win32ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "Manager/ControlUnitMgr.h"
#include "Manager/InputAgent.h"
#include "Manager/ScreencapAgent.h"
#include "Utils/Logger.h"
#include "Utils/SafeWindows.hpp"

const char* MaaWin32ControlUnitGetVersion()
{
#pragma message("MaaWin32ControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaControlUnitHandle MaaWin32ControlUnitCreate(void* hWnd, MaaWin32ScreencapMethod screencap_method, MaaWin32InputMethod input_method)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR_VOIDP(hWnd) << VAR(screencap_method) << VAR(input_method);

    if (!hWnd) {
        LogError << "hWnd is nullptr";
        return nullptr;
    }

    HWND h_wnd = reinterpret_cast<HWND>(hWnd);

    auto screencap_unit = std::make_shared<ScreencapAgent>(screencap_method, h_wnd);
    auto input_unit = std::make_shared<InputAgent>(input_method, h_wnd);

    auto unit_mgr = std::make_unique<ControlUnitMgr>(h_wnd, std::move(screencap_unit), std::move(input_unit));
    return unit_mgr.release();
}

void MaaWin32ControlUnitDestroy(MaaControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
