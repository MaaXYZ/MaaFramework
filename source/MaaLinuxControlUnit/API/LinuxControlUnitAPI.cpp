#include "MaaControlUnit/LinuxControlUnitAPI.h"

#include "Base/Config.h"
#include "MaaUtils/Logger.h"
#include "Manager/LinuxControlUnitMgr.h"

const char* MaaLinuxControlUnitGetVersion()
{
#pragma message("MaaLinuxControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaLinuxControlUnitHandle MaaLinuxControlUnitCreate(const char* config_json)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR(config_json);

    if (!config_json) {
        LogError << "config_json is null";
        return nullptr;
    }

    auto json_opt = json::parse(config_json);
    if (!json_opt) {
        LogError << "Failed to parse config_json" << VAR(config_json);
        return nullptr;
    }

    LinuxControlUnitConfig config = { };
    if (!config.from_json(*json_opt)) {
        LogError << "Failed to deserialize config" << VAR(*json_opt);
        return nullptr;
    }

    auto unit_mgr = std::make_unique<LinuxControlUnitMgr>(config);

    return unit_mgr.release();
}

void MaaLinuxControlUnitDestroy(MaaLinuxControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);

    if (handle) {
        delete handle;
    }
}
