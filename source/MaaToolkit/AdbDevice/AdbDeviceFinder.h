#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <meojson/json.hpp>

#include "AdbDeviceBuffer.hpp"
#include "Conf/Conf.h"
#include "MaaToolkit/MaaToolkitDef.h"
#include "Utils/Platform.h"

MAA_TOOLKIT_NS_BEGIN

class AdbDeviceFinder
{
public:
    struct EmulatorConstantData
    {
        std::string keyword;
        std::vector<std::filesystem::path> adb_candidate_paths;
        std::vector<std::string> adb_common_serials;
    };

    struct Emulator
    {
        std::string name;
        ProcessInfo process;
        const EmulatorConstantData const_data;
    };

public:
    virtual ~AdbDeviceFinder() = default;

public:
    std::vector<AdbDevice> find() const;
    std::vector<AdbDevice> find_specified(const std::filesystem::path& adb_path) const;

protected:
    virtual std::vector<std::string> find_adb_serials(const std::filesystem::path& adb_path, const Emulator& emulator) const;
    virtual json::object get_adb_config(const Emulator& emulator, const std::string& adb_serial) const;

protected:
    void set_emulator_const_data(std::unordered_map<std::string, EmulatorConstantData> data);

    std::vector<Emulator> find_emulators() const;
    std::filesystem::path get_adb_path(const EmulatorConstantData& emulator, os_pid pid) const;

    std::vector<std::string> find_serials_by_adb_command(const std::filesystem::path& adb_path) const;
    bool request_adb_connect(const std::filesystem::path& adb_path, const std::string& serial) const;
    std::vector<std::string>
        check_available_adb_serials(const std::filesystem::path& adb_path, const std::vector<std::string>& serials) const;
    MaaAdbScreencapMethod get_screencap_methods(const std::filesystem::path& adb_path, const std::string& serial) const;
    MaaAdbInputMethod get_input_methods(const std::filesystem::path& adb_path, const std::string& serial) const;

private:
    std::optional<std::vector<AdbDevice>> devices_;
    std::unordered_map<std::string, EmulatorConstantData> const_data_;
};

std::ostream& operator<<(std::ostream& os, const AdbDeviceFinder::Emulator& emulator);

MAA_TOOLKIT_NS_END
