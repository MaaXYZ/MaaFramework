#include "GlobalOptionMgr.h"

#include "MaaUtils/Logger.hpp"
#include "Utils/Platform.hpp"

MAA_NS_BEGIN

bool GlobalOptionMgr::set_option(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    switch (key) {
    case MaaGlobalOption_Logging:
        return set_logging(value, val_size);
    default:
        LogError << "Unknown key" << VAR(key) << VAR(value);
        return false;
    }
}

bool GlobalOptionMgr::set_logging(MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc;

    std::string_view str_path(reinterpret_cast<const char*>(value), val_size);
    auto path = MAA_NS::path(str_path);

    LogInfo << "Set logging path" << VAR(path);

    Logger::get_instance().start_logging(std::move(path));

    return true;
}

MAA_NS_END
