#if defined(_WIN32)
#include "AdbDeviceWin32Finder.h"

#include <unordered_map>

#include "Utils/IOStream/ChildPipeIOStream.h"
#include "Utils/Logger.h"
#include "Utils/StringMisc.hpp"

MAA_TOOLKIT_NS_BEGIN

using namespace path_literals;

AdbDeviceWin32Finder::AdbDeviceWin32Finder()
{
    std::unordered_map<std::string, EmulatorConstantData> emulators = {
        { "BlueStacks",
          { .keyword = "HD-Player",
            .adb_candidate_paths = { "HD-Adb.exe"_path, "Engine\\ProgramFiles\\HD-Adb.exe"_path },
            .adb_common_serials = { "127.0.0.1:5555", "127.0.0.1:5556", "127.0.0.1:5565", "127.0.0.1:5575" } } },
        { "LDPlayer",
          {
              .keyword = "dnplayer",
              .adb_candidate_paths = { "adb.exe"_path },
          } },
        { "Nox",
          { .keyword = "Nox",
            .adb_candidate_paths = { "nox_adb.exe"_path },
            .adb_common_serials = { "127.0.0.1:62001", "127.0.0.1:59865" } } },

        { "MuMuPlayer6",
          { .keyword = "NemuPlayer",
            .adb_candidate_paths = { "vmonitor\\bin\\adb_server.exe"_path,
                                     "MuMu\\emulator\\nemu\\vmonitor\\bin\\adb_server.exe"_path,
                                     "adb.exe"_path },
            .adb_common_serials = { "127.0.0.1:7555" } } },

        { "MuMuPlayer12",
          {
              .keyword = "MuMuPlayer.exe",
              .adb_candidate_paths = { "vmonitor\\bin\\adb_server.exe"_path,
                                       "MuMu\\emulator\\nemu\\vmonitor\\bin\\adb_server.exe"_path,
                                       "adb.exe"_path },
          } },

        { "MEmuPlayer", { .keyword = "MEmu", .adb_candidate_paths = { "adb.exe"_path }, .adb_common_serials = { "127.0.0.1:21503" } } },

        { "AVD",
          { .keyword = "qemu-system",
            .adb_candidate_paths = { "..\\..\\..\\platform-tools\\adb.exe"_path },
            .adb_common_serials = { "emulator-5554", "127.0.0.1:5555" } } },
    };

    set_emulator_const_data(std::move(emulators));
}

std::vector<std::string> AdbDeviceWin32Finder::find_adb_serials(const std::filesystem::path& adb_path, const Emulator& emulator) const
{
    if (emulator.name == "LDPlayer") {
        return find_serials_by_adb_command(adb_path);
    }
    else if (emulator.name == "MuMuPlayer12") {
        return find_mumu_serials(adb_path, emulator);
    }
    else {
        return AdbDeviceFinder::find_adb_serials(adb_path, emulator);
    }
}

json::object AdbDeviceWin32Finder::get_adb_config(const Emulator& emulator, const std::string& adb_serial) const
{
    if (emulator.name == "MuMuPlayer12") {
        // C:\Program Files\Netease\MuMuPlayer-12.0\shell\MuMuPlayer.exe
        auto path_opt = get_process_path(emulator.process.pid);
        if (!path_opt) {
            return {};
        }
        auto dir = path_opt->parent_path().parent_path();

        json::object cfg;
        auto& mumu_cfg = cfg["extras"]["mumu"];

        mumu_cfg["enable"] = true;
        mumu_cfg["path"] = path_to_utf8_string(dir);
        mumu_cfg["index"] = get_mumu_index(adb_serial);

        LogInfo << "MuMuPlayer12 cfg" << VAR(adb_serial) << cfg;
        return cfg;
    }
    else if (emulator.name == "LDPlayer") {
        // E:\Program Files\leidian\LDPlayer9\dnplayer.exe
        auto path_opt = get_process_path(emulator.process.pid);
        if (!path_opt) {
            return {};
        }
        auto dir = path_opt->parent_path();

        json::object cfg;
        auto& ld_cfg = cfg["extras"]["ld"];

        ld_cfg["enable"] = true;
        ld_cfg["path"] = path_to_utf8_string(dir);
        ld_cfg["index"] = get_ld_index(adb_serial);
        ld_cfg["pid"] = emulator.process.pid;

        LogInfo << "LDPlayer cfg" << VAR(adb_serial) << cfg;
        return cfg;
    }

    return {};
}

std::vector<std::string> AdbDeviceWin32Finder::find_mumu_serials(const std::filesystem::path& adb_path, const Emulator& emulator) const
{
    std::ignore = emulator;

    std::filesystem::path mumu_mgr_path = adb_path.parent_path() / "MuMuManager.exe";
    if (!std::filesystem::exists(mumu_mgr_path)) {
        LogWarn << "MuMuManager not found" << VAR(mumu_mgr_path);
        return find_serials_by_adb_command(adb_path);
    }

    static const std::vector<std::string> mumu_mgr_args = { "info", "--vmindex", "all" };
    ChildPipeIOStream ios(mumu_mgr_path, mumu_mgr_args);
    std::string output = ios.read();
    LogDebug << VAR(mumu_mgr_path) << VAR(mumu_mgr_args) << VAR(output);

    auto jopt = json::parse(output);
    if (!jopt) {
        LogError << "Parse MuMuManager info failed" << VAR(output);
        return {};
    }

    auto get_serial = [](const json::value& obj) -> std::optional<std::string> {
        auto ip_opt = obj.find<std::string>("adb_host_ip");
        auto port_opt = obj.find<int>("adb_port");
        if (!ip_opt || !port_opt) {
            return std::nullopt;
        }
        return std::format("{}:{}", *ip_opt, *port_opt);
    };

    std::vector<std::string> serials;
    auto unique_serial_opt = get_serial(*jopt);
    if (unique_serial_opt) {
        serials.emplace_back(*std::move(unique_serial_opt));
        return serials;
    }

    for (const auto& [key, obj] : jopt->as_object()) {
        auto serial_opt = get_serial(obj);
        if (!serial_opt) {
            continue;
        }
        serials.emplace_back(*std::move(serial_opt));
    }
    return serials;
}

int AdbDeviceWin32Finder::get_mumu_index(const std::string& adb_serial)
{
    auto sp = string_split(adb_serial, ':');
    if (sp.size() != 2) {
        return 0;
    }

    auto& str_port = sp.at(1);
    if (str_port.empty() || !std::ranges::all_of(str_port, [](auto c) { return std::isdigit(c); })) {
        return 0;
    }

    int port = std::stoi(str_port);
    int index = (port - 16384) / 32;

    return index;
}

int AdbDeviceWin32Finder::get_ld_index(const std::string& adb_serial)
{
    auto sp = string_split(adb_serial, '-');
    if (sp.size() != 2) {
        return 0;
    }

    auto& str_port = sp.at(1);
    if (str_port.empty() || !std::ranges::all_of(str_port, [](auto c) { return std::isdigit(c); })) {
        return 0;
    }

    int port = std::stoi(str_port);
    int index = (port - 5554) / 2;

    return index;
}

MAA_TOOLKIT_NS_END

#endif
