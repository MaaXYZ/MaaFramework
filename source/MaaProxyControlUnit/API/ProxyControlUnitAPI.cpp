#include "ControlUnit/ProxyControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "ProxyController.h"

const char* MaaProxyControlUnitGetVersion()
{
#pragma message("MaaProxyControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaControlUnitHandle MaaProxyControlUnitCreate(MaaControlUnitHandle inner, const char* dump_dir)
{
    LogFunc << VAR_VOIDP(inner) << VAR(dump_dir);

    if (!inner) {
        LogError << "inner is null";
        return nullptr;
    }

    if (!dump_dir) {
        LogError << "dump_dir is null";
        return nullptr;
    }

    return new MAA_CTRL_UNIT_NS::ProxyController(inner, MAA_NS::path(dump_dir));
}

void MaaProxyControlUnitDestroy(MaaControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
