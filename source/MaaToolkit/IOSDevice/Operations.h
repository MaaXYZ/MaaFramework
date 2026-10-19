#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>

#include "MaaFramework/MaaDef.h"
#include "wifi/PairRecord.h"
#include "json/Json.h"

namespace maa::ios::management
{
using Json = ::json::value;

struct Options
{
    std::string udid;
    std::string wifi;
    std::string cache_directory;
    std::string host_name;
    std::vector<wifi::PairRecord> pairing_records;
    bool replace = false;
    MaaIOSConnectionMethod connection_method = MaaIOSConnectionMethod_Auto;
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

bool resolve_connection(Options&, const Callbacks&, std::string& error);

Json discover(const Options&, const Callbacks&);
Json pair(const Options&, const Callbacks&, std::optional<wifi::PairRecord>& credential);
Json check_connection(const Options&, const Callbacks&);
Json prepare(const Options&, const Callbacks&);
} // namespace maa::ios::management
