#include "MaaControlUnit/AndroidNativeControlUnitAPI.h"

#include <memory>
#include <optional>
#include <utility>

#include <meojson/json.hpp>

#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "Manager/AndroidNativeControlUnitMgr.h"

using MAA_NS::path;

namespace
{
using namespace MAA_CTRL_UNIT_NS;

template <typename T>
bool get_optional_value(const json::value& input, const std::string& key, T& output, T default_value)
{
    if (auto opt = input.find<T>(key)) {
        output = *opt;
        return true;
    }

    if (input.exists(key)) {
        LogError << "Type error in config" << VAR(key) << VAR(input);
        return false;
    }

    output = default_value;
    return true;
}

std::optional<AndroidNativeControlUnitConfig> parse_config(const json::value& config)
{
    if (!config.is_object()) {
        LogError << "config_json is not an object" << VAR(config);
        return std::nullopt;
    }

    const auto library_path_opt = config.find<std::string>("library_path");
    if (!library_path_opt || library_path_opt->empty()) {
        LogError << "library_path is missing or empty" << VAR(config);
        return std::nullopt;
    }

    const auto screen_resolution_opt = config.find("screen_resolution");
    if (!screen_resolution_opt || !screen_resolution_opt->is_object()) {
        LogError << "screen_resolution is missing or not an object" << VAR(config);
        return std::nullopt;
    }

    const auto width_opt = screen_resolution_opt->find<int>("width");
    const auto height_opt = screen_resolution_opt->find<int>("height");
    if (!width_opt || !height_opt || *width_opt <= 0 || *height_opt <= 0) {
        LogError << "screen_resolution.width or screen_resolution.height is invalid" << VAR(config);
        return std::nullopt;
    }

    AndroidNativeControlUnitConfig result;
    result.library_path = path(*library_path_opt);
    result.touch_width = *width_opt;
    result.touch_height = *height_opt;

    if (!get_optional_value(config, "display_id", result.display_id, 0)) {
        return std::nullopt;
    }
    if (!get_optional_value(config, "force_stop", result.force_stop, false)) {
        return std::nullopt;
    }

    return result;
}
} // namespace

const char* MaaAndroidNativeControlUnitGetVersion()
{
#pragma message("MaaAndroidNativeControlUnit MAA_VERSION: " MAA_VERSION)

    return MAA_VERSION;
}

MaaAndroidNativeControlUnitHandle MaaAndroidNativeControlUnitCreate(const char* config_json)
{
    using namespace MAA_CTRL_UNIT_NS;

    LogFunc << VAR(config_json);

    if (!config_json) {
        LogError << "config_json is null";
        return nullptr;
    }

    auto json_opt = json::parse(config_json);
    if (!json_opt) {
        LogError << "Failed to parse config_json" << VAR(config_json);
        return nullptr;
    }

    auto config_opt = parse_config(*json_opt);
    if (!config_opt) {
        return nullptr;
    }

    auto unit_mgr = std::make_unique<AndroidNativeControlUnitMgr>(std::move(*config_opt));
    return unit_mgr.release();
}

void MaaAndroidNativeControlUnitDestroy(MaaAndroidNativeControlUnitHandle handle)
{
    LogFunc << VAR_VOIDP(handle);
    delete handle;
}
