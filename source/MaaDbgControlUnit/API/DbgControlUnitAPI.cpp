#include "ControlUnit/DbgControlUnitAPI.h"

#include <meojson/json.hpp>

#include "CarouselImage/CarouselImage.h"
#include "ReplayRecording/ReplayRecordingMgr.h"
#include "Utils/Logger.h"

MAA_DBG_CTRL_UNIT_NS_BEGIN

std::shared_ptr<ControllerAPI> create_controller(MaaDbgControllerType type, MaaStringView read_path,
                                                 MaaStringView write_path, MaaStringView config)
{
    LogFunc << VAR(type) << VAR(read_path) << VAR(write_path) << VAR(config);

    auto config_parsed = json::parse(config);
    if (!config_parsed) {
        LogError << "Failed to parse config" << VAR(config);
        return nullptr;
    }

    auto read_stdpath = MAA_NS::path(read_path);
    auto write_stdpath = MAA_NS::path(write_path);

    switch (type) {
    case MaaDbgControllerType_CarouselImage:
        return std::make_shared<CarouselImage>(read_stdpath);
    case MaaDbgControllerType_ReplayRecording:
        return create_replay_recording(read_stdpath);
    }

    return nullptr;
}

MAA_DBG_CTRL_UNIT_NS_END
