#if defined(__APPLE__)

#include "DeviceMgrMacOS.h"

#include <filesystem>
#include <map>
#include <ranges>

#include "Utils/IOStream/BoostIO.hpp"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_TOOLKIT_NS_BEGIN

using namespace path_literals;

struct EmulatorConstantData
{
    std::string keyword;
    std::vector<std::filesystem::path> adb_candidate_paths;
    std::vector<std::string> adb_common_serials;
};

std::filesystem::path get_adb_path(const EmulatorConstantData& emulator, os_pid pid);

static const std::map<std::string, EmulatorConstantData> kEmulators = {
    { "Nox",
      { .keyword = "Nox",
        .adb_candidate_paths = { "adb"_path },
        .adb_common_serials = { "127.0.0.1:62001", "127.0.0.1:59865" } } },
    { "MuMuPlayerPro",
      { .keyword = "MuMuEmulator",
        .adb_candidate_paths = { "tools/adb"_path },
        .adb_common_serials = { "127.0.0.1:16384", "127.0.0.1:16416" } } },
    { "AVD",
      { .keyword = "qemu-system",
        .adb_candidate_paths = { "../../../platform-tools/adb"_path },
        .adb_common_serials = { "emulator-5554", "127.0.0.1:5555" } } },
};

std::vector<Device> DeviceMgrMacOS::find_device_impl()
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
            // TODO: 根据设备情况使用不同的配置
            device.adb_config = json::object().to_string();
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

std::vector<Device> DeviceMgrMacOS::find_device_with_adb_impl(std::filesystem::path adb_path)
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

std::vector<DeviceMgrMacOS::Emulator> DeviceMgrMacOS::find_emulators() const
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

std::filesystem::path get_adb_path(const EmulatorConstantData& emulator, os_pid pid)
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

MAA_TOOLKIT_NS_END

#endif