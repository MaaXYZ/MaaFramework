#include "ControlUnit/DbgControlUnitAPI.h"

#include <meojson/json.hpp>

#include "CarouselImage/CarouselImage.h"
#include "ReplayRecording/ReplayRecordingMgr.h"
#include "Utils/Logger.h"

MaaStringView get_version()
{
#pragma message("MaaDbgControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaControlUnitHandle create_control_unit(MaaDbgControllerType type, MaaStringView read_path)
{
    LogFunc << VAR(type) << VAR(read_path);

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

void destroy_control_unit(MaaControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
