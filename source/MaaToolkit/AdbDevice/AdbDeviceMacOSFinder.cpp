#if defined(__APPLE__)

#include "AdbDeviceMacOSFinder.h"

#include <unordered_map>

MAA_TOOLKIT_NS_BEGIN

using namespace path_literals;

const AdbDeviceFinder::EmulatorConstDataMap& AdbDeviceMacOSFinder::get_emulator_const_data() const
{
    static const EmulatorConstDataMap kConstData {
        { "Nox",
          { .keyword = "Nox", .adb_candidate_paths = { "adb"_path }, .adb_common_serials = { "127.0.0.1:62001", "127.0.0.1:59865" } } },

        { "MuMuPlayerPro",
          { .keyword = "MuMuEmulator",
            .adb_candidate_paths = { "tools/adb"_path },
            .adb_common_serials = { "127.0.0.1:16384", "127.0.0.1:16416" } } },

        { "AVD",
          { .keyword = "qemu-system",
            .adb_candidate_paths = { "../../../platform-tools/adb"_path },
            .adb_common_serials = { "emulator-5554", "127.0.0.1:5555" } } },

        { "Genymotion",
          { .keyword = "genymotion",
            .adb_candidate_paths = { "player.app/Contents/MacOS/tools/adb"_path },
            .adb_common_serials = { "127.0.0.1:6555" } } },

        { "BlueStacks",
          { .keyword = "BlueStacks",
            .adb_candidate_paths = { "hd-adb"_path, "BlueStacks.app/Contents/MacOS/hd-adb"_path },
            .adb_common_serials = { "127.0.0.1:5555", "127.0.0.1:5556", "127.0.0.1:5565", "127.0.0.1:5575" } } },
    };

    return kConstData;
}

MAA_TOOLKIT_NS_END

#endif
