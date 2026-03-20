#include "ControlUnit/RecordControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "RecordController.h"

const char* MaaRecordControlUnitGetVersion()
{
#pragma message("MaaRecordControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaRecordControlUnitHandle MaaRecordControlUnitCreate(void* shared_inner, const char* dump_dir)
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
    return new MAA_CTRL_UNIT_NS::RecordController(inner, MAA_NS::path(dump_dir));
}

void MaaRecordControlUnitDestroy(MaaRecordControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
