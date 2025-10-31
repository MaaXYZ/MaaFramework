#include "OptionMgr.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_GLOBAL_NS_BEGIN

bool OptionMgr::set_option(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    switch (key) {
    case MaaGlobalOption_LogDir:
        return set_log_dir(value, val_size);
    case MaaGlobalOption_SaveDraw:
        return set_save_draw(value, val_size);
    case MaaGlobalOption_StdoutLevel:
        return set_stdout_level(value, val_size);
    case MaaGlobalOption_DebugMode:
        return set_debug_mode(value, val_size);
    default:
        LogError << "Unknown key" << VAR(key) << VAR(value);
        return false;
    }
}

bool OptionMgr::set_log_dir(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    std::string_view str_path(reinterpret_cast<const char*>(value), val_size);
    log_dir_ = MAA_NS::path(str_path);

    LogInfo << "Set log dir" << VAR(log_dir_);

    MAA_LOG_NS::Logger::get_instance().start_logging(log_dir_);

    return true;
}

bool OptionMgr::set_save_draw(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    if (val_size != sizeof(bool)) {
        LogError << "Invalid value size" << VAR(val_size);
        return false;
    }

    save_draw_ = *reinterpret_cast<const bool*>(value);

    LogInfo << "Set save draw" << VAR(save_draw_);

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

bool OptionMgr::set_debug_mode(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    if (val_size != sizeof(bool)) {
        LogError << "Invalid value size" << VAR(val_size);
        return false;
    }

    debug_mode_ = *reinterpret_cast<const bool*>(value);

    LogInfo << "Set debug message" << VAR(debug_mode_);

    return true;
}

MAA_GLOBAL_NS_END
