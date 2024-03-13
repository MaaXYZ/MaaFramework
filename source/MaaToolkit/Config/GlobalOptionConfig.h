#pragma once
#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class GlobalOptionConfig : public SingletonHolder<GlobalOptionConfig>
{
    friend class SingletonHolder<GlobalOptionConfig>;

public:
    inline static const std::filesystem::path kConfigPath_Deprecated = "config/maa_toolkit.json";
    inline static const std::filesystem::path kConfigPath = "config/maa_option.json";
    inline static const std::filesystem::path kDebugDir = "debug";

public:
    struct Option
    {
        bool logging = true;
        bool save_draw = false;
        bool recording = false;
        int32_t stdout_level = MaaLoggingLevel_Error;
        bool show_hit_draw = false;

        MEO_JSONIZATION(
            MEO_OPT logging,
            MEO_OPT save_draw,
            MEO_OPT recording,
            MEO_OPT stdout_level,
            MEO_OPT show_hit_draw);
    };

public:
    bool init(const std::filesystem::path& user_path, const json::value& default_config);

private:
    GlobalOptionConfig() = default;

    bool load();
    bool apply_option();

    bool save() const;

private:
    std::filesystem::path config_path_;
    std::filesystem::path debug_dir_;

private:
    Option option_;
};

MAA_TOOLKIT_NS_END
