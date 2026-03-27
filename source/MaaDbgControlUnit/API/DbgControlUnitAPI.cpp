#include "MaaControlUnit/DbgControlUnitAPI.h"

#include "DbgController.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

const char* MaaDbgControlUnitGetVersion()
{
#pragma message("MaaDbgControlUnit MAA_VERSION: " MAA_VERSION)
    return MAA_VERSION;
}

MaaDbgControlUnitHandle MaaDbgControlUnitCreate(const char* read_path)
{
    LogFunc << VAR(read_path);

    if (!read_path) {
        LogError << "read_path is null";
        return nullptr;
    }

    return new MAA_CTRL_UNIT_NS::DbgController(MAA_NS::path(read_path));
}

void MaaDbgControlUnitDestroy(MaaDbgControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
