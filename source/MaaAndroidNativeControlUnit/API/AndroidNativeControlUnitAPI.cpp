#include "ControlUnit/AndroidNativeControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "Manager/AndroidNativeControlUnitMgr.h"

const char* MaaAndroidNativeControlUnitGetVersion()
{
#pragma message("MaaAndroidNativeControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaAndroidNativeControlUnitHandle MaaAndroidNativeControlUnitCreate()
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc;

    auto unit_mgr = new AndroidNativeControlUnitMgr();
    return unit_mgr;
}

void MaaAndroidNativeControlUnitDestroy(MaaAndroidNativeControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
