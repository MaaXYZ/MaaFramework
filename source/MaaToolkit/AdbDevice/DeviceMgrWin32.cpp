#if defined(_WIN32)

#include "Utils/SafeWindows.hpp"

#include "DeviceMgrWin32.h"

#include <filesystem>
#include <map>
#include <ranges>

#include <Psapi.h>

#include "Utils/IOStream/BoostIO.hpp"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/StringMisc.hpp"

MAA_TOOLKIT_NS_BEGIN

using namespace path_literals;

const std::map<std::string, DeviceMgrWin32::EmulatorConstantData> DeviceMgrWin32::kEmulators = {
    { "BlueStacks",
      { .keyword = "HD-Player",
        .adb_candidate_paths = { "HD-Adb.exe"_path, "Engine\\ProgramFiles\\HD-Adb.exe"_path },
        .adb_common_serials = { "127.0.0.1:5555",
                                "127.0.0.1:5556",
                                "127.0.0.1:5565",
                                "127.0.0.1:5575" } } },

    { "LDPlayer",
      { .keyword = "dnplayer",
        .adb_candidate_paths = { "adb.exe"_path },
        .adb_common_serials = { "emulator-5554",
                                "emulator-5556",
                                "127.0.0.1:5555",
                                "127.0.0.1:5556" } } },

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
      { .keyword = "MuMuPlayer",
        .adb_candidate_paths = { "vmonitor\\bin\\adb_server.exe"_path,
                                 "MuMu\\emulator\\nemu\\vmonitor\\bin\\adb_server.exe"_path,
                                 "adb.exe"_path },
        .adb_common_serials = { "127.0.0.1:16384", "127.0.0.1:16416", "127.0.0.1:16448" } } },

    { "MEmuPlayer",
      { .keyword = "MEmu",
        .adb_candidate_paths = { "adb.exe"_path },
        .adb_common_serials = { "127.0.0.1:21503" } } },
};

std::vector<Device> DeviceMgrWin32::find_device_impl()
{
    std::vector<Device> result;

    auto all_emulators = find_emulators();
    for (const Emulator& e : all_emulators) {
        const auto& constant = kEmulators.at(e.name);
        std::filesystem::path adb_path = get_adb_path(constant, e.process.pid);

        auto serials = request_adb_serials(adb_path, json::object());
        serials.insert(
            serials.end(),
            constant.adb_common_serials.begin(),
            constant.adb_common_serials.end());
        // Deduplication
        auto set = std::set<std::string>(serials.begin(), serials.end());
        serials.assign(set.begin(), set.end());
        serials = check_available_adb_serials(adb_path, serials, json::object());

        for (const std::string& ser : serials) {
            Device device;
            device.name = e.name;
            device.adb_path = path_to_utf8_string(adb_path);
            device.adb_serial = ser;
            device.adb_config = get_adb_config(e, ser).to_string();
            device.adb_controller_type =
                check_adb_controller_type(device.adb_path, device.adb_serial, device.adb_config);
            result.emplace_back(std::move(device));
        }
    }

    auto env_adb = boost::process::search_path("adb");

    if (std::filesystem::exists(env_adb)) {
        auto env_adb_devices = find_device_with_adb_impl(path_to_utf8_string(env_adb));
        result.insert(
            result.end(),
            std::make_move_iterator(env_adb_devices.begin()),
            std::make_move_iterator(env_adb_devices.end()));
    }

    // 去重
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    return result;
}

std::vector<Device> DeviceMgrWin32::find_device_with_adb_impl(std::filesystem::path adb_path)
{
    std::vector<Device> result;

    auto serials = request_adb_serials(adb_path, json::object());

    for (const std::string& ser : serials) {
        Device device;
        device.adb_path = path_to_utf8_string(adb_path);
        device.name = device.adb_path;
        device.adb_serial = ser;
        device.adb_config = json::object().to_string();
        device.adb_controller_type =
            check_adb_controller_type(adb_path, device.adb_serial, device.adb_config);
        result.emplace_back(std::move(device));
    }
    return result;
}

std::vector<DeviceMgrWin32::Emulator> DeviceMgrWin32::find_emulators() const
{
    std::vector<Emulator> result;

    auto all_processes = list_processes();
    for (const auto& process : all_processes) {
        auto find_it = std::ranges::find_if(kEmulators, [&process](const auto& pair) -> bool {
            return process.name.find(pair.second.keyword) != std::string::npos;
        });
        if (find_it == kEmulators.cend()) {
            continue;
        }

        Emulator emulator {
            .name = find_it->first,
            .process = process,
        };
        result.emplace_back(std::move(emulator));
    }

    LogInfo << VAR(result);

    return result;
}

std::filesystem::path DeviceMgrWin32::get_adb_path(const EmulatorConstantData& emulator, os_pid pid)
{
    auto path_opt = get_process_path(pid);
    if (!path_opt) {
        return {};
    }
    auto dir = path_opt->parent_path();

    for (const auto& adb_rel_path : emulator.adb_candidate_paths) {
        auto adb_path = dir / adb_rel_path;
        if (!std::filesystem::exists(adb_path)) {
            continue;
        }
        return adb_path;
    }
    return {};
}

json::object DeviceMgrWin32::get_adb_config(const Emulator& emulator, const std::string& adb_serial)
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

        LogInfo << "Mumu cfg" << VAR(adb_serial) << cfg;
        return cfg;
    }

    return {};
}

int DeviceMgrWin32::get_mumu_index(const std::string& adb_serial)
{
    auto sp = string_split(adb_serial, ':');
    if (sp.size() != 2) {
        return 0;
    }

    auto& str_port = sp.at(1);
    if (str_port.empty()
        || !std::ranges::all_of(str_port, [](auto c) { return std::isdigit(c); })) {
        return 0;
    }

    int port = std::stoi(str_port);
    int index = (port - 16384) / 32;

    return index;
}

MAA_TOOLKIT_NS_END

#endif
