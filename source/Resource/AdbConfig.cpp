#include "AdbConfig.h"

#include <meojson/json.hpp>

#include "Utils/Logger.hpp"

MAA_RES_NS_BEGIN

bool AdbConfig::load(const std::filesystem::path& path, bool is_base)
{
    LogFunc << VAR(path) << VAR(is_base);

    auto opt = json::open(path);
    if (!opt) {
        LogError << "Open config file failed: " << path;
        return false;
    }

    auto& json = *opt;
    auto& command = json.at("Command");

    auto parse_command = [&](const std::string& key, std::vector<std::string>& value) {
        auto opt = command.find<json::value>(key);
        if (opt && opt->is_array()) {
            const auto& arr = opt->as_array();
            if (std::any_of(arr.begin(), arr.end(), [](const json::value& val) { return !val.is_string(); })) {
                LogError << "Parse config failed:" << VAR(key);
                return false;
            }
            value.clear();
            std::transform(arr.begin(), arr.end(), std::back_insert_iterator(value),
                           [](const json::value& val) { return val.as_string(); });
        }
        else if (is_base) {
            LogError << "Parse config failed:" << VAR(key);
            return false;
        }
        return true;
    };

    bool ret = parse_command("Connect", data_.connect) && parse_command("KillServer", data_.kill_server) &&
               parse_command("Uuid", data_.uuid) && parse_command("Resolution", data_.resolution) &&
               parse_command("StartApp", data_.start_app) && parse_command("StopApp", data_.stop_app) &&
               parse_command("Click", data_.click) && parse_command("Swipe", data_.swipe) &&
               parse_command("PressKey", data_.press_key) &&
               parse_command("ScreencapRawByNetcat", data_.screencap_raw_by_netcat) &&
               parse_command("NetcatAddress", data_.netcat_address) &&
               parse_command("ScreencapRawWithGzip", data_.screencap_raw_with_gzip) &&
               parse_command("ScreencapEncode", data_.screencap_encode) &&
               parse_command("ScreencapEncodeToFile", data_.screencap_encode_to_file) &&
               parse_command("PullFile", data_.pull_file) && parse_command("Abilist", data_.abilist) &&
               parse_command("Orientation", data_.orientation) && parse_command("PushBin", data_.push_bin) &&
               parse_command("ChmodBin", data_.chmod_bin) && parse_command("InvokeBin", data_.invoke_bin) &&
               parse_command("InvokeApp", data_.invoke_app);

    return ret;
}

MAA_RES_NS_END
