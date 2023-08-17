#if defined(_WIN32)

#include "Utils/SafeWindows.hpp"

#include "DeviceMgrWin32.h"

#include <filesystem>
#include <map>

#include <Psapi.h>

#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/Ranges.hpp"

MAA_TOOLKIT_DEVICE_NS_BEGIN

std::ostream& operator<<(std::ostream& os, const DeviceMgrWin32::Emulator& emulator)
{
    os << VAR_RAW(emulator.name) << VAR_RAW(emulator.process_name) << VAR_RAW(emulator.pid);
    return os;
}

static const std::vector<DeviceMgrWin32::Emulator> kEmulators = {
    { .name = "BlueStacks",
      .process_keyword = "HD-Player",
      .adb_relative_paths = { path("HD-Adb.exe"), path("Engine\\ProgramFiles\\HD-Adb.exe") },
      .adb_common_serials = { "127.0.0.1:5555", "127.0.0.1:5556", "127.0.0.1:5565", "127.0.0.1:5575", "127.0.0.1:5585",
                              "127.0.0.1:5595", "127.0.0.1:5554" } },

    { .name = "LDPlayer",
      .process_keyword = "dnplayer",
      .adb_relative_paths = { path("adb.exe") },
      .adb_common_serials = { "emulator-5554", "emulator-5556", "emulator-5558", "emulator-5560", "127.0.0.1:5555",
                              "127.0.0.1:5556", "127.0.0.1:5554" } },

    { .name = "Nox",
      .process_keyword = "Nox",
      .adb_relative_paths = { path("nox_adb.exe") },
      .adb_common_serials = { "127.0.0.1:62001", "127.0.0.1:59865" } },

    { .name = "MuMuPlayer6",
      .process_keyword = "NemuPlayer",
      .adb_relative_paths = { path("vmonitor\\bin\\adb_server.exe"),
                              path("MuMu\\emulator\\nemu\\vmonitor\\bin\\adb_server.exe"), path("adb.exe") },
      .adb_common_serials = { "127.0.0.1:7555" } },

    { .name = "MuMuPlayer12",
      .process_keyword = "MuMuPlayer",
      .adb_relative_paths = { path("vmonitor\\bin\\adb_server.exe"),
                              path("MuMu\\emulator\\nemu\\vmonitor\\bin\\adb_server.exe"), path("adb.exe") },
      .adb_common_serials = { "127.0.0.1:16384", "127.0.0.1:16416", "127.0.0.1:16448", "127.0.0.1:16480",
                              "127.0.0.1:16512", "127.0.0.1:16544", "127.0.0.1:16576" } },

    { .name = "MEmuPlayer",
      .process_keyword = "MEmu",
      .adb_relative_paths = { path("adb.exe") },
      .adb_common_serials = { "127.0.0.1:21503" } },
};

size_t DeviceMgrWin32::find_device(std::string_view adb_path)
{
    // TODO
    std::ignore = adb_path;

    auto emulators = get_emulators();

    return 0;
}

std::vector<DeviceMgrWin32::Emulator> DeviceMgrWin32::get_emulators()
{
    std::vector<Emulator> result;

    auto all_processes = list_processes();
    for (const auto& process : all_processes) {
        auto find_it = MAA_RNS::ranges::find_if(kEmulators, [&process](const auto& emulator) -> bool {
            return process.name.find(emulator.process_keyword) != std::string::npos;
        });
        if (find_it == kEmulators.cend()) {
            continue;
        }

        auto emulator = *find_it;
        emulator.pid = process.pid;
        emulator.process_name = process.name;

        result.emplace_back(std::move(emulator));
    }

    LogInfo << VAR(result);

    return result;
}

MAA_TOOLKIT_DEVICE_NS_END

#endif
