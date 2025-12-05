#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <meojson/json.hpp>

#include "AdbDeviceBuffer.hpp"
#include "Common/Conf.h"
#include "MaaToolkit/MaaToolkitDef.h"
#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/Platform.h"

MAA_TOOLKIT_NS_BEGIN

class AdbDeviceFinder
{
public:
    struct EmulatorConstantData
    {
        std::string keyword;
        std::vector<std::filesystem::path> adb_candidate_paths;
        std::vector<std::string> adb_common_serials;

        MEO_TOJSON(keyword, adb_candidate_paths, adb_common_serials);
    };

    struct Emulator
    {
        std::string name;
        std::filesystem::path process_path;
        std::filesystem::path adb_path;

        MEO_TOJSON(name, adb_path);
    };

public:
    virtual ~AdbDeviceFinder() = default;

public:
    std::vector<AdbDevice> find() const;
    std::vector<AdbDevice> find_specified(const std::filesystem::path& adb_path) const;

protected:
    virtual std::vector<AdbDevice> find_by_emulator_tool(const Emulator& emulator) const;

protected:
    void set_emulator_const_data(std::unordered_map<std::string, EmulatorConstantData> data);

    std::vector<std::string> find_serials_by_adb_command(const std::filesystem::path& adb_path) const;
    std::optional<AdbDevice> try_device(const std::filesystem::path& adb_path, const std::string& serial) const;

    std::vector<Emulator> find_emulators() const;
    std::filesystem::path get_emulator_adb_path(const EmulatorConstantData& emulator, os_pid pid) const;

private:
    std::unordered_map<std::string, EmulatorConstantData> const_data_;
};

MAA_TOOLKIT_NS_END
