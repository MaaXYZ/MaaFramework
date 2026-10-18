#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>

#include "wifi/PairRecord.h"
#include <nlohmann/json.hpp>

namespace maa::ios::management
{
using Json = nlohmann::json;

struct Options
{
    std::string udid;
    std::string wifi;
    std::string cache_directory;
    std::string host_name;
    std::vector<wifi::PairRecord> pairing_records;
    bool repair = false;
    bool replace = false;
    bool usb_only = false;
    bool wifi_only = false;
    int timeout_ms = 3000;
    int download_timeout_seconds = 300;
    int install_timeout_seconds = 180;
    unsigned short port = 49152;
};

struct Callbacks
{
    std::function<bool(const Json&)> event;
    std::function<bool()> cancelled;

    bool notify(const Json& value) const { return !event || event(value); }

    bool stopped() const { return cancelled && cancelled(); }
};

Json discover(const Options&, const Callbacks&);
Json pair(const Options&, const Callbacks&, std::string& credential);
Json check_connection(const Options&, const Callbacks&);
Json prepare(const Options&, const Callbacks&);
} // namespace maa::ios::management
