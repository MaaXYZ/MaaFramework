#include "MaaToolkit/MaaToolkitAPI.h"

#include "Config/ConfigMgr.h"
#include "Utils/Logger.h"

static MaaToolkitConfigMgrAPI& config_mgr = MAA_TOOLKIT_NS::ConfigMgr::get_instance();

MaaBool MaaToolkitInit()
{
    LogFunc;

    return config_mgr.init();
}

MaaBool MaaToolkitUninit()
{
    LogFunc;

    return config_mgr.uninit();
}
