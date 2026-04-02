#include "MaaControlUnit/RecordControlUnitAPI.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "RecordController.h"

const char* MaaRecordControlUnitGetVersion()
{
#pragma message("MaaRecordControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaRecordControlUnitHandle MaaRecordControlUnitCreate(void* shared_inner, const char* recording_path)
{
    LogFunc << VAR_VOIDP(shared_inner) << VAR(recording_path);

    if (!shared_inner) {
        LogError << "shared_inner is null";
        return nullptr;
    }

    if (!recording_path) {
        LogError << "recording_path is null";
        return nullptr;
    }

    auto& inner = *static_cast<std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI>*>(shared_inner);
    return new MAA_CTRL_UNIT_NS::RecordController(inner, MAA_NS::path(recording_path));
}

void MaaRecordControlUnitDestroy(MaaRecordControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
