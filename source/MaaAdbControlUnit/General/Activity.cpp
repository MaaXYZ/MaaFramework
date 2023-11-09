#include "Activity.h"

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

bool Activity::parse(const json::value& config)
{
    return parse_argv("StartApp", config, start_app_argv_) && parse_argv("StopApp", config, stop_app_argv_);
}

bool Activity::start_app(const std::string& intent)
{
    LogFunc;

    merge_replacement({ { "{INTENT}", intent } });
    auto cmd_ret = command(start_app_argv_.gen(argv_replace_));

    return cmd_ret.has_value();
}

bool Activity::stop_app(const std::string& intent)
{
    LogFunc;

    merge_replacement({ { "{INTENT}", intent } });
    auto cmd_ret = command(stop_app_argv_.gen(argv_replace_));

    return cmd_ret.has_value();
}

MAA_CTRL_UNIT_NS_END
