#include "ControlUnit/AndroidControlUnitAPI.h"

#include <meojson/json.hpp>

#include "MaaUtils/Logger.h"
#include "Manager/AndroidControlUnitMgr.h"

const char* MaaAndroidControlUnitGetVersion()
{
#pragma message("MaaAndroidControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaAndroidControlUnitHandle MaaAndroidControlUnitCreate(
    MaaAndroidScreencapMethod screencap_methods,
    MaaAndroidInputMethod input_methods)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR(screencap_methods) << VAR(input_methods);

    auto unit_mgr = std::make_unique<AndroidControlUnitMgr>(screencap_methods, input_methods);

    return unit_mgr.release();
}

void MaaAndroidControlUnitDestroy(MaaAndroidControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
