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

    auto parse_string = [&](const std::string& key, std::string& value) {
        auto opt = command.find<std::string>(key);
        if (opt) {
            value = *opt;
        }
        else if (is_base) {
            LogError << "Parse config failed:" << VAR(key);
            return false;
        }
        return true;
    };

    bool ret = parse_string("Connect", data_.connect) && parse_string("KillServer", data_.kill_server) &&
               parse_string("Uuid", data_.uuid) && parse_string("Resolution", data_.resolution) &&
               parse_string("StartApp", data_.start_app) && parse_string("StopApp", data_.stop_app) &&
               parse_string("Click", data_.click) && parse_string("Swipe", data_.swipe) &&
               parse_string("PressKey", data_.press_key) &&
               parse_string("ScreencapRawByNetcat", data_.screencap_raw_by_netcat) &&
               parse_string("NetcatAddress", data_.netcat_address) &&
               parse_string("ScreencapRawWithGzip", data_.screencap_raw_with_gzip) &&
               parse_string("ScreencapEncode", data_.screencap_encode) &&
               parse_string("ScreencapEncodeToFile", data_.screencap_encode_to_file) &&
               parse_string("PullFile", data_.pull_file) && parse_string("Abilist", data_.abilist) &&
               parse_string("Orientation", data_.orientation) && parse_string("PushBin", data_.push_bin) &&
               parse_string("ChmodBin", data_.chmod_bin) && parse_string("InvokeBin", data_.invoke_bin) &&
               parse_string("InvokeApp", data_.invoke_app);

    return ret;
}

MAA_RES_NS_END
