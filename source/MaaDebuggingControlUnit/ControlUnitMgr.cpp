#include "ControlUnitMgr.h"

#include <meojson/json.hpp>

#include "General/VirtualInfo.h"
#include "Screencap/ReadIndex.h"
#include "Utils/Logger.h"

#pragma message("MaaControlUnit MAA_VERSION: " MAA_VERSION)

MAA_DBG_CTRL_UNIT_NS_BEGIN

std::shared_ptr<ControlUnitAPI> create_controller_unit(MaaStringView read_path, MaaStringView wirte_path,
                                                       MaaDebuggingControllerType type, MaaStringView config)
{
    LogFunc << VAR(read_path) << VAR(wirte_path) << VAR(type);

    auto filepath = MAA_NS::path(read_path);
    std::ignore = wirte_path;

    std::shared_ptr<ScreencapAPI> screencap_unit = nullptr;
    auto screencap_type = type & MaaDebuggingControllerType_Screencap_Mask;

    switch (screencap_type) {
    case MaaDebuggingControllerType_Screencap_ReadIndex:
        screencap_unit = std::make_shared<ReadIndex>(filepath);
        break;
    }

    auto device_info_unit = std::make_shared<VirtualInfo>(filepath);
    bool parsed = device_info_unit->parse(config);
    if (!parsed) {
        LogError << "failed to parse config" << VAR(config);
        return nullptr;
    }

    auto unit_mgr = std::make_shared<ControlUnitMgr>();
    unit_mgr->set_screencap_obj(std::move(screencap_unit));
    unit_mgr->set_device_info_obj(std::move(device_info_unit));

    return unit_mgr;
}

MAA_DBG_CTRL_UNIT_NS_END
