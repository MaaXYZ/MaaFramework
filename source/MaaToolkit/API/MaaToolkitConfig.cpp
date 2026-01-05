#include "MaaToolkit/Config/MaaToolkitConfig.h"

#include <meojson/json.hpp>

#include "Config/GlobalOptionConfig.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/Runtime.h"

MaaBool MaaToolkitConfigInitOption(const char* user_path, const char* default_json)
{
    LogInfo << VAR(user_path) << VAR(default_json);

    if (!user_path) {
        LogError << "user_path is null";
        return false;
    }

    if (!default_json) {
        LogError << "default_json is null";
        return false;
    }

    auto json_opt = json::parse(default_json);
    if (!json_opt) {
        LogError << "failed to parse json" << default_json;
        return false;
    }

    auto& config = MAA_TOOLKIT_NS::GlobalOptionConfig::get_instance();
    return config.init(MAA_NS::path(user_path), *json_opt);
}
