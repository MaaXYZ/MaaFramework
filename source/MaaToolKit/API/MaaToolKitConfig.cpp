#include "MaaToolKit/MaaToolKitAPI.h"

#include "Config/ConfigMgr.h"
#include "Utils/Logger.h"

static MaaToolKitConfigMgrAPI& config_mgr = MAA_TOOLKIT_CONFIG_NS::ConfigMgr::get_instance();

MaaBool MaaToolKitInit()
{
    LogFunc;

    return config_mgr.init();
}

MaaBool MaaToolKitUninit()
{
    LogFunc;

    return config_mgr.uninit();
}
