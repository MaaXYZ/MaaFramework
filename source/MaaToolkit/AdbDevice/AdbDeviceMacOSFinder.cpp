#if defined(__APPLE__)

#include "AdbDeviceMacOSFinder.h"

#include <unordered_map>

MAA_TOOLKIT_NS_BEGIN

using namespace path_literals;

AdbDeviceMacOSFinder::AdbDeviceMacOSFinder()
{
    std::unordered_map<std::string, EmulatorConstantData> emulators = {
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
    };

    set_emulator_const_data(std::move(emulators));
}

MAA_TOOLKIT_NS_END

#endif
