#include "MaaToolkit/MaaToolkitAPI.h"

#include <meojson/json.hpp>

#include "Config/GlobalOptionConfig.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/Runtime.h"

MaaBool MaaToolkitInitOptionConfig(MaaStringView user_path, MaaStringView default_json)
{
    LogInfo << VAR(user_path) << VAR(default_json);

    auto json_opt = json::parse(default_json);
    if (!json_opt) {
        LogError << "failed to parse json" << default_json;
        return false;
    }

    auto& config = MAA_TOOLKIT_NS::GlobalOptionConfig::get_instance();
    return config.init(MAA_NS::path(user_path), *json_opt);
}

MaaBool MaaToolkitInit()
{
    LogWarn << __FUNCTION__ << "is deprecated, use MaaToolkitInitOptionConfig instead.";

    std::string user_path = MAA_NS::path_to_utf8_string(MAA_NS::library_dir());

    return MaaToolkitInitOptionConfig(user_path.c_str(), "{}");
}

MaaBool MaaToolkitUninit()
{
    LogWarn << __FUNCTION__ << "is deprecated, don't use it.";

    return true;
}
