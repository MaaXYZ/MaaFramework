#pragma once

#include <functional>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace maa::ios::tool
{
enum class Command
{
    Help,
    Devices,
    Pair,
    Connect,
    Prepare
};

struct Options
{
    Command command = Command::Help;
    std::string udid;
    std::string wifi;
    std::string cache_directory;
    bool json = false;
    bool repair = false;
    bool replace = false;
    bool usb_only = false;
    bool wifi_only = false;
    int timeout_ms = 0;
    int download_timeout_seconds = 300;
    int install_timeout_seconds = 180;
    unsigned short port = 49152;
};

bool parse(const std::vector<std::string_view>& arguments, Options& options, std::string& error);
void help(std::ostream& output);
std::string terminal_text(std::string_view text);
std::string json_string(std::string_view text);
using Execute = std::function<int(const Options&, std::ostream&, std::ostream&, const std::function<bool()>&)>;
int run(const Options& options, std::ostream& output, std::ostream& errors, const std::function<bool()>& cancelled, const Execute& execute);
}
