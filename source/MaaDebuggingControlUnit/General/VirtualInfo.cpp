#include "ControlUnitMgr.h"

#include "Utils/ImageIo.h"
#include "Utils/Logger.h"
#include "VirtualInfo.h"

MAA_DBG_CTRL_UNIT_NS_BEGIN

bool VirtualInfo::parse(const json::value& config)
{
    auto device_info = config.find("device_info");
    if (!device_info) {
        LogError << "device_info not found" << VAR(config);
        return false;
    }

    uuid_ = device_info->get("uuid", path_to_utf8_string(path_));
    resolution_.width = device_info->get("screen_width", 0);
    resolution_.height = device_info->get("screen_height", 0);
    orientation_ = device_info->get("orientation", 0);

    return true;
}

MAA_DBG_CTRL_UNIT_NS_END
