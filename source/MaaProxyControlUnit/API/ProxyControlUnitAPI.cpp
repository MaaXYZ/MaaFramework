#include "ControlUnit/ProxyControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "ProxyController.h"

const char* MaaProxyControlUnitGetVersion()
{
#pragma message("MaaProxyControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaControlUnitHandle MaaProxyControlUnitCreate(void* shared_inner, const char* dump_dir)
{
    LogFunc << VAR_VOIDP(shared_inner) << VAR(dump_dir);

    if (!shared_inner) {
        LogError << "shared_inner is null";
        return nullptr;
    }

    if (!dump_dir) {
        LogError << "dump_dir is null";
        return nullptr;
    }

    auto& inner = *static_cast<std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI>*>(shared_inner);
    return new MAA_CTRL_UNIT_NS::ProxyController(inner, MAA_NS::path(dump_dir));
}

void MaaProxyControlUnitDestroy(MaaControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
