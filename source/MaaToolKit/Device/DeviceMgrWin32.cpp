#if defined(_WIN32)

#include "Utils/Platform/SafeWindows.h"

#include "DeviceMgrWin32.h"

#include <filesystem>
#include <map>

#include <Psapi.h>

#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"
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
    // https://learn.microsoft.com/en-us/windows/win32/psapi/enumerating-all-processes
    DWORD all_pids[4096] = { 0 };
    DWORD pid_written = 0;

    if (!EnumProcesses(all_pids, sizeof(all_pids), &pid_written)) {
        auto error = GetLastError();
        LogError << "Failed to EnumProcesses" << VAR(error);
        return {};
    }
    DWORD size = pid_written / sizeof(DWORD);
    LogDebug << "Process size:" << size;

    std::vector<Emulator> result;
    std::vector<std::string> all_proc_names;
    WCHAR process_name_buff[MAX_PATH] = { 0 };

    for (DWORD i = 0; i < size; ++i) {
        DWORD pid = all_pids[i];
        if (pid == 0) {
            continue;
        }

        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (process == nullptr) {
            auto error = GetLastError();
            LogError << "Failed to OpenProcess" << VAR(error) << VAR(pid);
            continue;
        }

        HMODULE mod = nullptr;
        DWORD mod_written = 0;
        if (!EnumProcessModules(process, &mod, sizeof(mod), &mod_written)) {
            auto error = GetLastError();
            LogError << "Failed to EnumProcessModules" << VAR(error) << VAR(pid);
            continue;
        }

        memset(process_name_buff, 0, sizeof(process_name_buff));
        GetModuleBaseNameW(process, mod, process_name_buff, sizeof(process_name_buff) / sizeof(TCHAR));
        std::string process_name = from_osstring(process_name_buff);

        all_proc_names.emplace_back(process_name);

        auto find_it = MAA_RNS::ranges::find_if(kEmulators, [&process_name](const auto& emulator) -> bool {
            return process_name.find(emulator.process_keyword) != std::string::npos;
        });
        if (find_it == kEmulators.cend()) {
            continue;
        }

        auto emulator = *find_it;
        emulator.pid = pid;
        emulator.process_name = std::move(process_name);

        result.emplace_back(std::move(emulator));
    }

#ifdef MAA_DEBUG
    LogInfo << VAR(all_proc_names);
#endif
    LogInfo << VAR(result);

    return result;
}

MAA_TOOLKIT_DEVICE_NS_END

#endif
