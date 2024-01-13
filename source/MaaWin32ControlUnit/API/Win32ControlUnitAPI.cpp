#include "ControlUnit/Win32ControlUnitAPI.h"

#include "Base/UnitBase.h"
#include "Input/SendMessageInput.h"
#include "Manager/ControlUnitMgr.h"
// #include "Screencap/BackBufferScreencap.h"
#include "Screencap/DesktopDupScreencap.h"
#include "Screencap/FramePoolScreencap.h"
#include "Screencap/GdiScreencap.h"
#include "Utils/Logger.h"
#include "Utils/SafeWindows.hpp"

MaaStringView MaaWin32ControlUnitGetVersion()
{
#pragma message("MaaWin32ControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaControlUnitHandle MaaWin32ControlUnitCreate( //
    MaaWin32Hwnd hWnd, MaaWin32ControllerType type, MaaControllerCallback callback,
    MaaCallbackTransparentArg callback_arg)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR_VOIDP(hWnd) << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    if (!hWnd) {
        LogError << "hWnd is nullptr";
        return nullptr;
    }
    HWND h_wnd = reinterpret_cast<HWND>(hWnd);

    std::shared_ptr<TouchInputBase> touch_unit = nullptr;
    std::shared_ptr<KeyInputBase> key_unit = nullptr;
    std::shared_ptr<ScreencapBase> screencap_unit = nullptr;

    auto touch_type = type & MaaWin32ControllerType_Touch_Mask;
    // auto key_type = type & MaaWin32ControllerType_Key_Mask;
    auto screencap_type = type & MaaWin32ControllerType_Screencap_Mask;

    switch (touch_type) {
    case MaaWin32ControllerType_Touch_SendMessage:
        LogInfo << "touch_type: SendMessage";
        touch_unit = std::make_shared<SendMessageInput>(h_wnd);
        break;

    default:
        LogWarn << "Unknown touch input type" << VAR(touch_type);
        break;
    }

    switch (screencap_type) {
    case MaaWin32ControllerType_Screencap_GDI:
        LogInfo << "screencap_type: GDI";
        screencap_unit = std::make_shared<GdiScreencap>(h_wnd);
        break;

    case MaaWin32ControllerType_Screencap_DXGI_DesktopDup:
        LogInfo << "screencap_type: DXGI_DesktopDup";
        screencap_unit = std::make_shared<DesktopDupScreencap>();
        break;

        // Not work
        // case MaaWin32ControllerType_Screencap_DXGI_BackBuffer:
        //     LogInfo << "screencap_type: DXGI_BackBuffer";
        //     screencap_unit = std::make_shared<BackBufferScreencap>(h_wnd);
        //     break;

#if MAA_FRAMEPOOL_SCREENCAP_AVAILABLE
    case MaaWin32ControllerType_Screencap_DXGI_FramePool:
        LogInfo << "screencap_type: DXGI_FramePool";
        screencap_unit = std::make_shared<FramePoolScreencap>(h_wnd);
        break;
#endif

    default:
        LogWarn << "Unknown screencap input type" << VAR(screencap_type);
        break;
    }

    auto unit_mgr = std::make_unique<ControlUnitMgr>(h_wnd, callback, callback_arg);

    unit_mgr->set_touch_input_obj(touch_unit);
    unit_mgr->set_key_input_obj(key_unit);
    unit_mgr->set_screencap_obj(screencap_unit);

    return unit_mgr.release();
}

void MaaWin32ControlUnitDestroy(MaaControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
