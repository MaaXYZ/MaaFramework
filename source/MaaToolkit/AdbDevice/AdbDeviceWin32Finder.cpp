#if defined(_WIN32)
#include "AdbDeviceWin32Finder.h"

#include <unordered_map>

#include "MaaUtils/IOStream/ChildPipeIOStream.h"
#include "MaaUtils/Logger.h"
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

    return {};
}

std::vector<AdbDevice> AdbDeviceWin32Finder::find_mumu_devices(const Emulator& emulator) const
{
    LogFunc << VAR(emulator.name);

    if (emulator.adb_path.empty() || !std::filesystem::exists(emulator.adb_path)) {
        LogWarn << "adb_path is empty or does not exist" << VAR(emulator.adb_path);
        return {};
    }

    std::filesystem::path mumu_mgr_path = emulator.adb_path.parent_path() / "MuMuManager.exe";
    if (!std::filesystem::exists(mumu_mgr_path)) {
        LogWarn << "MuMuManager not found" << VAR(mumu_mgr_path);
        return {};
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
        return {};
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
        device.input_methods = MaaAdbInputMethod_Default | MaaAdbInputMethod_EmulatorExtras;

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
        return {};
    }

    // E:\Program Files\leidian\LDPlayer9\dnplayer.exe
    auto dir = emulator.process_path.parent_path();

    std::filesystem::path ldconsole_path = dir / "ldconsole.exe";
    if (!std::filesystem::exists(ldconsole_path)) {
        LogWarn << "ldconsole.exe not found" << VAR(ldconsole_path);
        return {};
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

MAA_TOOLKIT_NS_END

#endif
