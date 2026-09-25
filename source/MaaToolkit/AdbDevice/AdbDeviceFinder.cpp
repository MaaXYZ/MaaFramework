#include "AdbDeviceFinder.h"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <ranges>
#include <unordered_set>

#include "LibraryHolder/ControlUnit.h"
#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaUtils/IOStream/BoostIO.hpp"
#include "MaaUtils/Logger.h"
#include "MaaUtils/StringMisc.hpp"

MAA_TOOLKIT_NS_BEGIN

namespace
{
void append_unique_devices(
    std::vector<AdbDevice>& result,
    std::unordered_set<std::string>& accurate_serials,
    std::vector<AdbDevice> devices)
{
    for (auto& dev : devices) {
        if (!accurate_serials.emplace(dev.serial).second) {
            continue;
        }
        result.emplace_back(std::move(dev));
    }
}

// 端口被防火墙丢包时 adb connect 不会立刻失败，而是等到 Connection::connect_remote() 的 60s 超时；
// 枚举阶段先用短超时探一次，避免一次扫描被多个无响应端口卡住。
bool is_tcp_port_open(const std::string& serial)
{
    std::string_view view(serial);
    auto colon = view.rfind(':');
    if (colon == std::string_view::npos) {
        return true; // emulator-5554 之类的名字没有端口可探测，交给 adb 自行判断
    }

    boost::system::error_code ec;
    auto address = boost::asio::ip::make_address(view.substr(0, colon), ec);
    if (ec) {
        return true; // 不认识的地址形式不做拦截
    }

    auto port_view = view.substr(colon + 1);
    int port = 0;
    auto [ptr, port_ec] = std::from_chars(port_view.data(), port_view.data() + port_view.size(), port);
    if (port_ec != std::errc { } || ptr != port_view.data() + port_view.size() || port <= 0 || port > 65535) {
        return true;
    }

    constexpr auto kProbeTimeout = std::chrono::milliseconds(500);

    boost::asio::io_context context;
    boost::asio::ip::tcp::socket socket(context);
    bool open = false;
    socket.async_connect(
        boost::asio::ip::tcp::endpoint(address, static_cast<std::uint16_t>(port)),
        [&](const boost::system::error_code& connect_ec) { open = !connect_ec; });
    context.run_for(kProbeTimeout);
    return open;
}
} // namespace

std::vector<AdbDevice> AdbDeviceFinder::find() const
{
    LogFunc;

    std::vector<AdbDevice> result;
    std::unordered_set<std::string> accurate_serials;

    auto all_emulators = find_emulators();
    for (const Emulator& e : all_emulators) {
        auto res = find_by_emulator_tool(e);
        bool found = !res.empty();
        append_unique_devices(result, accurate_serials, std::move(res));
        if (found) {
            continue;
        }

        if (e.adb_path.empty() || !std::filesystem::exists(e.adb_path)) {
            LogWarn << "adb_path is empty or does not exist" << VAR(e.adb_path);
            continue;
        }

        append_unique_devices(result, accurate_serials, find_specified(e.adb_path, accurate_serials, e));
        append_unique_devices(result, accurate_serials, find_by_common_serials(e.adb_path, accurate_serials, e));
    }

    if (auto env_adb = boost::process::search_path("adb"); std::filesystem::exists(env_adb)) {
        append_unique_devices(result, accurate_serials, find_specified(env_adb, accurate_serials));
    }

    LogInfo << VAR(result);
    return result;
}

std::vector<AdbDevice> AdbDeviceFinder::find_specified(
    const std::filesystem::path& adb_path,
    const std::unordered_set<std::string>& exclude_serials,
    const Emulator& emulator) const
{
    LogFunc << VAR(adb_path);

    std::vector<AdbDevice> result;

    auto serials = find_serials_by_adb_command(adb_path);

    for (const std::string& ser : serials) {
        if (exclude_serials.count(ser)) {
            LogInfo << "skip excluded serial" << VAR(ser);
            continue;
        }
        auto res_opt = try_device(adb_path, ser, emulator);
        if (!res_opt) {
            continue;
        }
        result.emplace_back(std::move(*res_opt));
    }

    LogInfo << VAR(result);
    return result;
}

std::vector<AdbDevice> AdbDeviceFinder::find_by_common_serials(
    const std::filesystem::path& adb_path,
    const std::unordered_set<std::string>& exclude_serials,
    const Emulator& emulator) const
{
    LogFunc << VAR(adb_path) << VAR(emulator.common_serials);

    std::vector<AdbDevice> result;

    // adb server 与模拟器之间的 TCP 会话可能已断开（模拟器进程与端口仍在），此时 adb devices 不会列出设备，
    // 需要按已知端口重新 connect 一次。
    for (const std::string& ser : emulator.common_serials) {
        if (exclude_serials.count(ser)) {
            continue;
        }
        if (!is_tcp_port_open(ser)) {
            LogInfo << "skip unreachable common serial" << VAR(ser);
            continue;
        }
        auto res_opt = try_device(adb_path, ser, emulator);
        if (!res_opt) {
            continue;
        }
        result.emplace_back(std::move(*res_opt));
    }

    LogInfo << VAR(result);
    return result;
}

std::vector<std::string> AdbDeviceFinder::find_serials_by_adb_command(const std::filesystem::path& adb_path) const
{
    LogFunc << VAR(adb_path);

    std::string str_adb = path_to_utf8_string(adb_path);

    auto control_unit =
        AdbControlUnitLibraryHolder::create_control_unit(str_adb.c_str(), "", MaaAdbScreencapMethod_None, MaaAdbInputMethod_None, "{}", "");

    if (!control_unit) {
        LogError << "Failed to create control unit";
        return { };
    }

    std::vector<std::string> devices;
    auto found = control_unit->find_device(devices);

    LogDebug << VAR(devices);

    if (!found) {
        LogError << "Failed to find_device";
        return { };
    }

    return devices;
}

bool request_waydroid_config(std::shared_ptr<MAA_CTRL_UNIT_NS::AdbControlUnitAPI> control_unit, AdbDevice& device)
{
    if (!control_unit) {
        return false;
    }

    std::string output;
    bool ret = control_unit->shell("getprop | grep ro.product.brand", output);
    if (!ret) {
        return false;
    }

    string_trim_(output);
    tolowers_(output);

    if (output.find("waydroid") == std::string::npos) {
        return false;
    }

    auto& command = device.config["command"];

    command["StartApp"] = json::array {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "monkey -p {INTENT} --pct-syskeys 0 1",
    };
    command["StartActivity"] = json::array {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "am start -n {INTENT} --windowingMode 4",
    };

    LogInfo << "waydroid" << VAR(device);
    return true;
}

bool request_androws_config(std::shared_ptr<MAA_CTRL_UNIT_NS::AdbControlUnitAPI> control_unit, AdbDevice& device)
{
    if (!control_unit) {
        return false;
    }

    // Detect via Tencent-specific property; non-empty means this is an Androws device
    std::string output;
    bool ret = control_unit->shell("getprop sys.tencent.imei", output);
    if (!ret) {
        return false;
    }

    if (output.find_first_not_of(" \t\n\r\f\v") == std::string::npos) {
        return false;
    }

    // Just mark the device; AndrowsExtras will dynamically resolve the correct
    // display ID at runtime using the app package name.
    device.config["extras"]["androws"]["enable"] = true;
    device.screencap_methods = MaaAdbScreencapMethod_EmulatorExtras;
    device.input_methods = MaaAdbInputMethod_EmulatorExtras;

    LogInfo << "Androws detected" << VAR(device);
    return true;
}

bool request_avd_config(std::shared_ptr<MAA_CTRL_UNIT_NS::AdbControlUnitAPI> control_unit, AdbDevice& device)
{
    if (!control_unit) {
        return false;
    }

    if (!device.serial.starts_with("emulator-")) {
        return false;
    }

    std::string output;
    if (!control_unit->shell("getprop ro.product.model", output)) {
        return false;
    }

    string_trim_(output);
    tolowers_(output);

    if (!output.starts_with("android sdk") && !output.starts_with("sdk_")) {
        return false;
    }

    device.config["extras"]["avd"]["enable"] = true;

    LogInfo << "AVDExtras enabled for" << VAR(device);
    return true;
}

bool request_vivo_orientation_config(std::shared_ptr<MAA_CTRL_UNIT_NS::AdbControlUnitAPI> control_unit, AdbDevice& device)
{
    if (!control_unit) {
        return false;
    }

    std::string brand;
    if (!control_unit->shell("getprop ro.product.brand", brand)) {
        return false;
    }

    string_trim_(brand);
    tolowers_(brand);

    std::string manufacturer;
    if (control_unit->shell("getprop ro.product.manufacturer", manufacturer)) {
        string_trim_(manufacturer);
        tolowers_(manufacturer);
    }

    std::string model;
    if (control_unit->shell("getprop ro.product.model", model)) {
        string_trim_(model);
        tolowers_(model);
    }

    const bool is_vivo_device = brand.find("vivo") != std::string::npos || manufacturer.find("vivo") != std::string::npos
                                || model.find("vivo") != std::string::npos || model.find("iqoo") != std::string::npos;

    if (!is_vivo_device) {
        return false;
    }

    // If SurfaceOrientation exists, the default Orientation command should still work.
    // In this case, do not override the default command to minimize the impact scope.
    std::string surface_orientation;
    if (control_unit->shell("dumpsys input | grep -m 1 SurfaceOrientation", surface_orientation)) {
        string_trim_(surface_orientation);
        if (!surface_orientation.empty()) {
            return false;
        }
    }

    // Some vivo / iQOO devices do not have SurfaceOrientation in dumpsys input,
    // but Viewport INTERNAL contains orientation=0/1/2/3.
    std::string viewport;
    if (!control_unit->shell("dumpsys input | grep -m 1 'Viewport INTERNAL'", viewport)) {
        return false;
    }

    if (viewport.find("orientation=") == std::string::npos) {
        return false;
    }

    auto& command = device.config["command"];

    command["Orientation"] = json::array {
        "{ADB}",
        "-s",
        "{ADB_SERIAL}",
        "shell",
        "dumpsys input | sed -n 's/.*Viewport INTERNAL.*orientation=\\([0-3]\\).*/\\1/p' | tail -n 1",
    };

    LogInfo << "vivo orientation config enabled" << VAR(device);
    return true;
}

std::optional<AdbDevice>
    AdbDeviceFinder::try_device(const std::filesystem::path& adb_path, const std::string& serial, const Emulator& emulator) const
{
    LogFunc << VAR(adb_path) << VAR(serial);

    std::string str_adb = path_to_utf8_string(adb_path);

    auto control_unit = AdbControlUnitLibraryHolder::create_control_unit(
        str_adb.c_str(),
        serial.c_str(),
        MaaAdbScreencapMethod_None,
        MaaAdbInputMethod_None,
        "{}",
        "");

    if (!control_unit) {
        LogError << "Failed to create control unit";
        return std::nullopt;
    }
    if (!control_unit->connect()) {
        LogError << "Failed to connect" << VAR(serial);
        return std::nullopt;
    }

    AdbDevice device;
    device.name = std::format("{}-{}", serial, emulator.name.empty() ? path_to_utf8_string(adb_path) : emulator.name);
    device.adb_path = adb_path;
    device.serial = serial;
    device.screencap_methods = MaaAdbScreencapMethod_Default;
    device.input_methods = MaaAdbInputMethod_Default;
    device.config = { };

    if (request_waydroid_config(control_unit, device)) {
    }
    else if (request_androws_config(control_unit, device)) {
    }
    else if (request_avd_config(control_unit, device)) {
    }
    else if (request_vivo_orientation_config(control_unit, device)) {
    }
    // else if (request_xxx_config(control_unit, device)) {
    // }
    else {
    }

    return device;
}

std::vector<AdbDeviceFinder::Emulator> AdbDeviceFinder::find_emulators() const
{
    LogFunc;

    std::vector<Emulator> result;
    std::unordered_set<std::filesystem::path> seen_adb_paths;

    auto all_processes = list_processes();

    const auto& emu_constant = get_emulator_const_data();

    for (const auto& process : all_processes) {
        auto find_it = std::ranges::find_if(emu_constant, [&process](const auto& pair) -> bool {
            return process.name.find(pair.second.keyword) != std::string::npos;
        });
        if (find_it == emu_constant.cend()) {
            continue;
        }

        auto process_path = get_process_path(process.pid);
        if (!process_path) {
            continue;
        }

        auto adb_path = get_emulator_adb_path(find_it->second, process.pid);

        if (adb_path.empty()) {
            LogWarn << "adb_path is empty or does not exist" << VAR(adb_path);
            continue;
        }

        // Deduplicate by adb_path to distinguish multiple instances or installations
        if (!seen_adb_paths.insert(adb_path).second) {
            continue;
        }

        Emulator emulator {
            .name = find_it->first,
            .process_path = *process_path,
            .adb_path = adb_path,
            .common_serials = find_it->second.adb_common_serials,
        };
        result.emplace_back(std::move(emulator));
    }

    // Platform-specific fallback discovery (e.g. registry lookup for elevated-process emulators like Androws).
    // Merged here so downstream logic in find() treats them uniformly. Deduplicated by adb_path against
    // process-enumerated entries to avoid connecting to the same adb server twice.
    for (auto& extra : find_extra_emulators()) {
        if (extra.adb_path.empty() || !std::filesystem::exists(extra.adb_path)) {
            LogWarn << "extra emulator has invalid adb_path" << VAR(extra);
            continue;
        }
        if (!seen_adb_paths.insert(extra.adb_path).second) {
            continue;
        }
        result.emplace_back(std::move(extra));
    }

    LogInfo << VAR(result);

    return result;
}

std::filesystem::path AdbDeviceFinder::get_emulator_adb_path(const EmulatorConstantData& emulator, os_pid pid) const
{
    auto path_opt = get_process_path(pid);
    if (!path_opt) {
        return { };
    }
    auto dir = path_opt->parent_path();

    for (const auto& adb_rel_path : emulator.adb_candidate_paths) {
        auto adb_path = dir / adb_rel_path;
        if (!std::filesystem::exists(adb_path)) {
            continue;
        }
        return std::filesystem::canonical(adb_path);
    }
    return { };
}

MAA_TOOLKIT_NS_END
