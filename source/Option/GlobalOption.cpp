#include "GlobalOption.h"

#include "MaaParam.h"
#include "Utils/Platform.hpp"
#include "Utils/Logger.hpp"

MAA_NS_BEGIN

bool GlabalOption::set_option(std::string_view key, std::string_view value)
{
    LogInfo << VAR(key) << VAR(value);

    if (key == MaaGlobalOption_Logging) {
        Logger::get_instance().start_logging(path(value));
    }
    else {
        LogError << "Unknown key" << VAR(key) << VAR(value);
        return false;
    }

    return true;
}

MAA_NS_END
