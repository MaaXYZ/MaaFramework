#include <iostream>

#include "LibraryHolder/ControlUnit.h"

int main()
{
#ifdef WITH_ADB_CONTROLLER

    std::cout << "********** AdbControlUnitLibraryHolder::create_control_unit **********" << std::endl;
    auto adb_handle = MAA_NS::AdbControlUnitLibraryHolder::create_control_unit(
        "adb",
        "127.0.0.1:5555",
        MaaAdbScreencapMethod_Default,
        MaaAdbInputMethod_Default,
        "{}",
        "./");
    if (!adb_handle) {
        std::cerr << "Failed to create adb control unit" << std::endl;
        return -1;
    }

#endif

#ifdef WITH_WIN32_CONTROLLER

    std::cout << "********** Win32ControlUnitLibraryHolder::create_control_unit **********" << std::endl;
    auto win32_handle = MAA_NS::Win32ControlUnitLibraryHolder::create_control_unit(
        FindWindowA(nullptr, nullptr),
        MaaWin32ScreencapMethod_DXGI_DesktopDup,
        MaaWin32InputMethod_Seize,
        MaaWin32InputMethod_SendMessage);
    if (!win32_handle) {
        std::cerr << "Failed to create win32 control unit" << std::endl;
        return -1;
    }

#endif

#ifdef WITH_PLAYCOVER_CONTROLLER

    std::cout << "********** PlayCoverControlUnitLibraryHolder::create_control_unit **********" << std::endl;
    auto playcover_handle = MAA_NS::PlayCoverControlUnitLibraryHolder::create_control_unit("127.0.0.1:1717", "com.example.app");
    if (!playcover_handle) {
        std::cerr << "Failed to create playcover control unit" << std::endl;
        return -1;
    }

#endif

#ifdef WITH_DBG_CONTROLLER

    std::cout << "********** DbgControlUnitLibraryHolder::create_control_unit **********" << std::endl;
    auto dbg_handle = MAA_NS::DbgControlUnitLibraryHolder::create_control_unit(MaaDbgControllerType_CarouselImage, "./");
    if (!dbg_handle) {
        std::cerr << "Failed to create dbg control unit" << std::endl;
        return -1;
    }

#endif

#ifdef WITH_CUSTOM_CONTROLLER

    MaaCustomControllerCallbacks callbacks = {};

    std::cout << "********** CustomControlUnitLibraryHolder::create_control_unit **********" << std::endl;
    auto custom_handle = MAA_NS::CustomControlUnitLibraryHolder::create_control_unit(&callbacks, nullptr);
    if (!custom_handle) {
        std::cerr << "Failed to create custom control unit" << std::endl;
        return -1;
    }

#endif

    std::cout << "********** Done **********" << std::endl;
    return 0;
}
