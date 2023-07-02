#include "GlobalOptionMgr.h"

#include "MaaUtils/Logger.hpp"
#include "Utils/Platform.hpp"

MAA_NS_BEGIN

bool GlobalOptionMgr::set_option(MaaGlobalOption key, const std::string& value)
{
    LogInfo << VAR(key) << VAR(value);

    switch (key) {
    case MaaGlobalOption_Logging:
        Logger::get_instance().start_logging(path(value));
        return true;
    default:
        LogError << "Unknown key" << VAR(key) << VAR(value);
        return false;
    }
}

MAA_NS_END
