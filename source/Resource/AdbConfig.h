#pragma once

#include "Base/NonCopyable.hpp"
#include "Common/MaaConf.h"

#include <filesystem>

MAA_RES_NS_BEGIN

class AdbConfig : public NonCopyable
{
    struct Data
    {
        std::string connect;
        std::string kill_server;
        std::string uuid;
        std::string resolution;

        std::string start_app;
        std::string stop_app;

        std::string click;
        std::string swipe;
        std::string press_key;

        std::string screencap_raw_by_netcat;
        std::string netcat_address;
        std::string screencap_raw_with_gzip;
        std::string screencap_encode;
        std::string screencap_encode_to_file;
        std::string pull_file;

        std::string abilist;
        std::string orientation;
        std::string push_bin;
        std::string chmod_bin;
        std::string invoke_bin;
        std::string invoke_app;
    };

public:
    bool load(const std::filesystem::path& path, bool is_base);

    const Data& data() { return data_; }

private:
    Data data_;
};

MAA_RES_NS_END
