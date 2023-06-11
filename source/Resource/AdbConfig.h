#pragma once

#include "Base/NonCopyable.hpp"
#include "Common/MaaConf.h"

#include <filesystem>
#include <vector>

MAA_RES_NS_BEGIN

class AdbConfig : public NonCopyable
{
    struct Data
    {
        std::vector<std::string> connect;
        std::vector<std::string> kill_server;
        std::vector<std::string> uuid;
        std::vector<std::string> resolution;

        std::vector<std::string> start_app;
        std::vector<std::string> stop_app;

        std::vector<std::string> click;
        std::vector<std::string> swipe;
        std::vector<std::string> press_key;

        std::vector<std::string> screencap_raw_by_netcat;
        std::vector<std::string> netcat_address;
        std::vector<std::string> screencap_raw_with_gzip;
        std::vector<std::string> screencap_encode;
        std::vector<std::string> screencap_encode_to_file;
        std::vector<std::string> pull_file;

        std::vector<std::string> abilist;
        std::vector<std::string> orientation;
        std::vector<std::string> push_bin;
        std::vector<std::string> chmod_bin;
        std::vector<std::string> invoke_bin;
        std::vector<std::string> invoke_app;
    };

public:
    bool load(const std::filesystem::path& path, bool is_base);

    const Data& data() { return data_; }

private:
    Data data_;
};

MAA_RES_NS_END
