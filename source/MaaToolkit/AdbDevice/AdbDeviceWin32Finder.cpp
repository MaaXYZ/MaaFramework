#if defined(_WIN32)
#include "AdbDeviceWin32Finder.h"

#include <unordered_map>

#include "MaaUtils/IOStream/ChildPipeIOStream.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/SafeWindows.hpp"
#include "MaaUtils/StringMisc.hpp"

MAA_TOOLKIT_NS_BEGIN

using namespace path_literals;

const AdbDeviceFinder::EmulatorConstDataMap& AdbDeviceWin32Finder::get_emulator_const_data() const
{
    static const EmulatorConstDataMap kConstData {
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
        { "MuMuPlayer12 v5",
          {
              .keyword = "MuMuNxDevice.exe",
              .adb_candidate_paths = { "..\\..\\..\\nx_main\\adb.exe"_path, "adb.exe"_path },
          } },

        { "MEmuPlayer", { .keyword = "MEmu", .adb_candidate_paths = { "adb.exe"_path }, .adb_common_serials = { "127.0.0.1:21503" } } },

        { "AVD",
          { .keyword = "qemu-system",
            .adb_candidate_paths = { "..\\..\\..\\platform-tools\\adb.exe"_path },
            .adb_common_serials = { "emulator-5554", "127.0.0.1:5555" } } },

        { "Androws", { .keyword = "Androws", .adb_candidate_paths = { "adb.exe"_path } } },
    };

    return kConstData;
}

std::vector<AdbDevice> AdbDeviceWin32Finder::find_by_emulator_tool(const Emulator& emulator) const
{
    LogFunc << VAR(emulator.name);

    if (emulator.name == "MuMuPlayer12" || emulator.name == "MuMuPlayer12 v5") {
        return find_mumu_devices(emulator);
    }
    else if (emulator.name == "LDPlayer") {
        return find_ld_devices(emulator);
    }

    return { };
}

std::vector<AdbDevice> AdbDeviceWin32Finder::find_mumu_devices(const Emulator& emulator) const
{
    LogFunc << VAR(emulator.name);

    if (emulator.adb_path.empty() || !std::filesystem::exists(emulator.adb_path)) {
        LogWarn << "adb_path is empty or does not exist" << VAR(emulator.adb_path);
        return { };
    }

    std::filesystem::path mumu_mgr_path = emulator.adb_path.parent_path() / "MuMuManager.exe";
    if (!std::filesystem::exists(mumu_mgr_path)) {
        LogWarn << "MuMuManager not found" << VAR(mumu_mgr_path);
        return { };
    }

    static const std::vector<std::string> mumu_mgr_args = { "info", "--vmindex", "all" };
    ChildPipeIOStream ios(mumu_mgr_path, mumu_mgr_args);
    std::string output = ios.read();
    LogDebug << VAR(mumu_mgr_path) << VAR(mumu_mgr_args) << VAR(output);

    auto jopt = json::parse(output);
    if (!jopt) {
        LogError << "Parse MuMuManager info failed" << VAR(output);
        return { };
    }

    struct MumuInfo
    {
        std::string index;
        std::string name;
        std::string adb_host_ip;
        int adb_port;

        MEO_JSONIZATION(index, name, adb_host_ip, adb_port);
    };

    json::value& jinfo = *jopt;
    std::vector<MumuInfo> info;

    if (jinfo.is<MumuInfo>()) {
        info = { jinfo.as<MumuInfo>() };
    }
    else if (jinfo.is_object()) {
        for (const auto& i : jinfo.as_object() | std::views::values) {
            if (i.is<MumuInfo>()) {
                info.emplace_back(i.as<MumuInfo>());
            }
            else {
                LogWarn << "Invalid MuMuManager info format" << VAR(i);
                continue;
            }
        }
    }
    else {
        LogError << "Invalid MuMuManager info format" << VAR(output);
        return { };
    }

    std::filesystem::path dir;
    if (emulator.name == "MuMuPlayer12 v5") {
        // MuMuPlayer12 v5: C:\Program Files\Netease\MuMuPlayer-12.0\nx_device\12.0\shell\MuMuNxDevice.exe
        dir = emulator.process_path.parent_path().parent_path().parent_path().parent_path();
    }
    else {
        // MuMuPlayer12: C:\Program Files\Netease\MuMuPlayer-12.0\shell\MuMuPlayer.exe
        dir = emulator.process_path.parent_path().parent_path();
    }

    std::vector<AdbDevice> result;
    for (const MumuInfo& i : info) {
        AdbDevice device;
        device.name = std::format("{}-{}", i.name, emulator.name);
        device.adb_path = emulator.adb_path;
        device.serial = std::format("{}:{}", i.adb_host_ip, i.adb_port);

        device.screencap_methods = MaaAdbScreencapMethod_EmulatorExtras;
        device.input_methods = MaaAdbInputMethod_Default;

        int index = 0;
        if (std::ranges::all_of(i.index, [](unsigned char c) { return std::isdigit(c); })) {
            index = std::stoi(i.index);
        }
        else {
            LogError << "Invalid MuMu index" << VAR(i.index);
            continue;
        }

        auto& mumu_cfg = device.config["extras"]["mumu"];
        mumu_cfg["enable"] = true;
        mumu_cfg["path"] = path_to_utf8_string(dir);
        mumu_cfg["index"] = index;

        LogInfo << "MuMu device" << VAR(device);

        result.emplace_back(std::move(device));
    }

    return result;
}

std::vector<AdbDevice> AdbDeviceWin32Finder::find_ld_devices(const Emulator& emulator) const
{
    LogFunc << VAR(emulator.name);

    if (emulator.adb_path.empty() || !std::filesystem::exists(emulator.adb_path)) {
        LogWarn << "adb_path is empty or does not exist" << VAR(emulator.adb_path);
        return { };
    }

    // E:\Program Files\leidian\LDPlayer9\dnplayer.exe
    auto dir = emulator.process_path.parent_path();

    std::filesystem::path ldconsole_path = dir / "ldconsole.exe";
    if (!std::filesystem::exists(ldconsole_path)) {
        LogWarn << "ldconsole.exe not found" << VAR(ldconsole_path);
        return { };
    }

    static const std::vector<std::string> ldconsole_args = { "list2" };
    ChildPipeIOStream ios(ldconsole_path, ldconsole_args);
    std::string raw = ios.read();
    std::string output = gbk_to_utf8(raw);
    LogDebug << VAR(ldconsole_path) << VAR(ldconsole_args) << VAR(raw) << VAR(output);

    // list2 output format:
    // 索引,标题,顶层窗口句柄,绑定窗口句柄,是否进入android,进程PID,VBox进程PID
    // 0,雷电模拟器,2032678,1704928,1,7456,3500
    // 1,雷电模拟器-1,852422,590830,1,3772,3180

    std::vector<AdbDevice> result;
    auto lines = string_split(output, '\n');
    for (auto line : lines) {
        string_trim_(line);
        string_replace_all_(line, "\r", "");
        if (line.empty()) {
            continue;
        }

        auto fields = string_split(line, ',');
        if (fields.size() < 7) {
            LogWarn << "Invalid list2 output line" << VAR(line);
            continue;
        }

        int index = 0;
        int vbox_pid = 0;
        if (std::ranges::all_of(fields[0], [](unsigned char c) { return std::isdigit(c); })
            && std::ranges::all_of(fields[6], [](unsigned char c) { return std::isdigit(c); })) {
            index = std::stoi(fields[0]);
            vbox_pid = std::stoi(fields[6]);
        }
        else {
            LogWarn << "Failed to parse list2 fields" << VAR(line);
            continue;
        }

        if (vbox_pid <= 0) {
            LogWarn << "Invalid VBox PID" << VAR(line);
            continue;
        }

        // LDPlayer adb serial: emulator-5554, emulator-5556, ...
        std::string serial = std::format("emulator-{}", 5554 + index * 2);

        AdbDevice device;
        device.name = std::format("{}-{}", fields[1], emulator.name);
        device.adb_path = emulator.adb_path;
        device.serial = serial;

        device.screencap_methods = MaaAdbScreencapMethod_EmulatorExtras;
        device.input_methods = MaaAdbInputMethod_Default;

        auto& ld_cfg = device.config["extras"]["ld"];
        ld_cfg["enable"] = true;
        ld_cfg["path"] = path_to_utf8_string(dir);
        ld_cfg["index"] = index;
        ld_cfg["pid"] = vbox_pid;

        LogInfo << "LDPlayer device" << VAR(device);

        result.emplace_back(std::move(device));
    }

    return result;
}

std::vector<AdbDeviceFinder::Emulator> AdbDeviceWin32Finder::find_extra_emulators() const
{
    LogFunc;

    std::vector<Emulator> result;

    if (auto androws_adb = find_androws_adb_path_from_registry()) {
        Emulator emulator {
            .name = "Androws",
            .process_path = { },
            .adb_path = *androws_adb,
        };
        LogInfo << "Androws found via registry" << VAR(emulator);
        result.emplace_back(std::move(emulator));
    }

    return result;
}

namespace
{
// Read a REG_SZ value from an opened registry key into a std::wstring. Returns nullopt on any failure.
std::optional<std::wstring> read_reg_sz(HKEY key, const wchar_t* value_name)
{
    DWORD type = 0;
    DWORD size = 0;
    LONG ret = RegQueryValueExW(key, value_name, nullptr, &type, nullptr, &size);
    if (ret != ERROR_SUCCESS || type != REG_SZ || size == 0) {
        return std::nullopt;
    }

    // size is in bytes, may or may not include the trailing null.
    std::vector<BYTE> data(size);
    ret = RegQueryValueExW(key, value_name, nullptr, &type, data.data(), &size);
    if (ret != ERROR_SUCCESS || type != REG_SZ || size == 0) {
        return std::nullopt;
    }

    // Length in wchar_t; drop trailing nulls if present.
    size_t len = size / sizeof(wchar_t);
    auto* wbuf = reinterpret_cast<const wchar_t*>(data.data());
    while (len > 0 && wbuf[len - 1] == L'\0') {
        --len;
    }
    if (len == 0) {
        return std::nullopt;
    }
    return std::wstring(wbuf, len);
}
}

std::optional<std::filesystem::path> AdbDeviceWin32Finder::find_androws_adb_path_from_registry()
{
    HKEY install_key = nullptr;
    OnScopeLeave([&]() {
        if (install_key) {
            RegCloseKey(install_key);
        }
    });

    // Silent miss: treat "not installed" as normal; do not log at warn/error.
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Tencent\\Androws", 0, KEY_READ, &install_key) != ERROR_SUCCESS) {
        return std::nullopt;
    }

    auto install_path = read_reg_sz(install_key, L"InstallPath");
    if (!install_path) {
        LogWarn << "Androws registry key exists but InstallPath is missing or invalid";
        return std::nullopt;
    }

    HKEY app_key = nullptr;
    OnScopeLeave([&]() {
        if (app_key) {
            RegCloseKey(app_key);
        }
    });

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Tencent\\Androws\\Androws", 0, KEY_READ, &app_key) != ERROR_SUCCESS) {
        LogWarn << "Androws InstallPath present but sub key 'Androws' missing";
        return std::nullopt;
    }

    auto version = read_reg_sz(app_key, L"Version");
    if (!version) {
        LogWarn << "Androws sub key exists but Version is missing or invalid";
        return std::nullopt;
    }

    std::filesystem::path adb_path = std::filesystem::path(*install_path) / L"Application" / *version / L"adb.exe";
    if (!std::filesystem::exists(adb_path)) {
        LogWarn << "Androws resolved adb.exe does not exist" << VAR(adb_path);
        return std::nullopt;
    }

    return std::filesystem::canonical(adb_path);
}

MAA_TOOLKIT_NS_END

#endif
