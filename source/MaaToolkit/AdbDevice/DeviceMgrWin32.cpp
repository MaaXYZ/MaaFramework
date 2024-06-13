#if defined(_WIN32)
#include "DeviceMgrWin32.h"

#include <unordered_map>

#include "Utils/Logger.h"
#include "Utils/StringMisc.hpp"

MAA_TOOLKIT_NS_BEGIN

using namespace path_literals;

DeviceMgrWin32::DeviceMgrWin32()
{
    std::unordered_map<std::string, EmulatorConstantData> emulators = {
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

    set_emulator_const_data(std::move(emulators));
}

json::object
    DeviceMgrWin32::get_adb_config(const Emulator& emulator, const std::string& adb_serial) const
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
