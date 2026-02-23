#if defined(__linux__)

#include "AdbDeviceLinuxFinder.h"

MAA_TOOLKIT_NS_BEGIN

using namespace path_literals;

const AdbDeviceFinder::EmulatorConstDataMap& AdbDeviceLinuxFinder::get_emulator_const_data() const
{
    static const EmulatorConstDataMap kConstData {
        { "AVD",
          { .keyword = "qemu-system",
            .adb_candidate_paths = { "../../../platform-tools/adb"_path },
            .adb_common_serials = { "emulator-5554", "127.0.0.1:5555" } } },
    };

    return kConstData;
}

MAA_TOOLKIT_NS_END

#endif
