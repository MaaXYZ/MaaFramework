#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
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

    using EmulatorConstDataMap = std::unordered_map<std::string, EmulatorConstantData>;

    struct Emulator
    {
        std::string name;
        std::filesystem::path process_path;
        std::filesystem::path adb_path;

        MEO_TOJSON(name, process_path, adb_path);
    };

public:
    virtual ~AdbDeviceFinder() = default;

public:
    std::vector<AdbDevice> find() const;
    std::vector<AdbDevice> find_specified(
        const std::filesystem::path& adb_path,
        const std::unordered_set<std::string>& exclude_serials = {},
        const Emulator& emulator = {}) const;

protected:
    virtual const EmulatorConstDataMap& get_emulator_const_data() const { return kEmptyEmulatorConstDataMap; }

    virtual std::vector<AdbDevice> find_by_emulator_tool(const Emulator&) const { return {}; }

protected:
    std::vector<std::string> find_serials_by_adb_command(const std::filesystem::path& adb_path) const;
    std::optional<AdbDevice> try_device(const std::filesystem::path& adb_path, const std::string& serial, const Emulator& emulator) const;

    std::vector<Emulator> find_emulators() const;
    std::filesystem::path get_emulator_adb_path(const EmulatorConstantData& emulator, os_pid pid) const;

private:
    inline static const EmulatorConstDataMap kEmptyEmulatorConstDataMap;
};

MAA_TOOLKIT_NS_END
