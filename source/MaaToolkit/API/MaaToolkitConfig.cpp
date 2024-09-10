#include "MaaToolkit/Config/MaaToolkitConfig.h"

#include <meojson/json.hpp>

#include "Config/GlobalOptionConfig.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/Runtime.h"

MaaBool MaaToolkitConfigInitOption(const char* user_path, const char* default_json)
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
