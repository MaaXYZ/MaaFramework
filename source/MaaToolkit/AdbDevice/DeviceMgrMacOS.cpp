#if defined(__APPLE__)

#include "DeviceMgrMacOS.h"

#include <unordered_map>

MAA_TOOLKIT_NS_BEGIN

using namespace path_literals;

static const std::unordered_map<std::string, EmulatorConstantData> kEmulators = {
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

DeviceMgrMacOS::DeviceMgrMacOS()
{
    set_emulator_const_data(kEmulators);
}

MAA_TOOLKIT_NS_END

#endif
