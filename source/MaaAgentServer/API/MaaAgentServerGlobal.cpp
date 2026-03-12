#include "MaaFramework/MaaAPI.h"

#include "Global/OptionMgr.h"
#include "MaaUtils/Logger.h"

MaaBool MaaGlobalSetOption(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    return MAA_AGENT_SERVER_NS::OptionMgr::get_instance().set_option(key, value, val_size);
}

MaaBool MaaSetGlobalOption(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogError << "Deprecated API, use MaaGlobalSetOption instead";

    return MaaGlobalSetOption(key, value, val_size);
}
