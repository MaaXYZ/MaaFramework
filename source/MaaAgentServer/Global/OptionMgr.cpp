#include "OptionMgr.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_AGENT_SERVER_NS_BEGIN

bool OptionMgr::set_option(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    switch (key) {
    case MaaGlobalOption_LogDir:
        return set_log_dir(value, val_size);
    case MaaGlobalOption_StdoutLevel:
        return set_stdout_level(value, val_size);
    case MaaGlobalOption_LogLevel:
        return set_log_level(value, val_size);
    case MaaGlobalOption_LogCleanupDays:
        return set_log_cleanup_days(value, val_size);
    default:
        LogError << "MaaAgentServer does not support this option" << VAR(key);
        return false;
    }
}

bool OptionMgr::set_log_dir(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    std::string_view str_path(reinterpret_cast<const char*>(value), val_size);
    auto dir = MAA_NS::path(str_path);

    LogInfo << "Set log dir" << VAR(dir);

    MAA_LOG_NS::Logger::get_instance().start_logging(dir);

    return true;
}

bool OptionMgr::set_stdout_level(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    if (val_size != sizeof(MaaLoggingLevel)) {
        LogError << "Invalid value size" << VAR(val_size);
        return false;
    }

    MaaLoggingLevel level = *reinterpret_cast<const MaaLoggingLevel*>(value);

    LogInfo << "Set log stdout level" << VAR(level);

    MAA_LOG_NS::Logger::get_instance().set_stdout_level(static_cast<MAA_LOG_NS::level>(level));

    return true;
}

bool OptionMgr::set_log_level(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    if (val_size != sizeof(MaaLoggingLevel)) {
        LogError << "Invalid value size" << VAR(val_size);
        return false;
    }

    MaaLoggingLevel level = *reinterpret_cast<const MaaLoggingLevel*>(value);

    LogInfo << "Set log file level" << VAR(level);

    MAA_LOG_NS::Logger::get_instance().set_log_level(static_cast<MAA_LOG_NS::level>(level));

    return true;
}

bool OptionMgr::set_log_cleanup_days(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    if (val_size != sizeof(int32_t)) {
        LogError << "Invalid value size" << VAR(val_size);
        return false;
    }

    int32_t days = *reinterpret_cast<const int32_t*>(value);
    if (days < 0) {
        LogError << "Invalid log cleanup days, should be >= 0" << VAR(days);
        return false;
    }

    LogInfo << "Set log cleanup days" << VAR(days);

    MAA_LOG_NS::Logger::get_instance().set_log_cleanup_days(days);

    return true;
}

MAA_AGENT_SERVER_NS_END
