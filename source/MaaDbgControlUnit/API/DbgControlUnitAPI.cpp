#include "ControlUnit/DbgControlUnitAPI.h"

#include "CarouselImage/CarouselImage.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "ReplayRecording/ReplayRecordingMgr.h"

const char* MaaDbgControlUnitGetVersion()
{
#pragma message("MaaDbgControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaControlUnitHandle MaaDbgControlUnitCreate(MaaDbgControllerType type, const char* read_path)
{
    LogFunc << VAR(type) << VAR(read_path);

    if (!read_path) {
        LogError << "read_path is null";
        return nullptr;
    }

    auto read_stdpath = MAA_NS::path(read_path);

    MaaControlUnitHandle handle = nullptr;

    switch (type) {
    case MaaDbgControllerType_CarouselImage:
        handle = new MAA_CTRL_UNIT_NS::CarouselImage(read_stdpath);
        break;

    case MaaDbgControllerType_ReplayRecording:
        handle = MAA_CTRL_UNIT_NS::create_replay_recording(read_stdpath);
        break;
    }

    LogDebug << VAR_VOIDP(handle);

    return handle;
}

void MaaDbgControlUnitDestroy(MaaControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
