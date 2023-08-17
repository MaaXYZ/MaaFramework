#include "GlobalOptionMgr.h"

#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_NS_BEGIN

bool GlobalOptionMgr::set_option(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    switch (key) {
    case MaaGlobalOption_Logging:
        return set_logging(value, val_size);
    case MaaGlobalOption_DebugMode:
        return set_debug_mode(value, val_size);
    default:
        LogError << "Unknown key" << VAR(key) << VAR(value);
        return false;
    }
}

bool GlobalOptionMgr::set_logging(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    std::string_view str_path(reinterpret_cast<const char*>(value), val_size);
    logging_path_ = MAA_NS::path(str_path);

    LogInfo << "Set logging path" << VAR(logging_path_);

    Logger::get_instance().start_logging(logging_path_);

    return true;
}

bool GlobalOptionMgr::set_debug_mode(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    if (val_size != sizeof(bool)) {
        LogError << "Invalid value size" << VAR(val_size);
        return false;
    }

    debug_mode_ = *reinterpret_cast<const bool*>(value);

    LogInfo << "Set debug mode" << VAR(debug_mode_);

    return true;
}

MAA_NS_END
