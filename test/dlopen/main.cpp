#include <iostream>

#include "LibraryHolder/ControlUnit.h"

int main()
{
#ifdef WITH_ADB_CONTROLLER
    auto adb_type =
        MaaAdbControllerType_Input_Preset_Adb | MaaAdbControllerType_Screencap_FastestWay;
    std::cout << "********** AdbControlUnitLibraryHolder::create_control_unit **********"
              << std::endl;
    auto adb_handle = MAA_NS::AdbControlUnitLibraryHolder::create_control_unit(
        "adb",
        "127.0.0.1:5555",
        adb_type,
        "{}",
        "./",
        nullptr,
        nullptr);
    if (!adb_handle) {
        std::cerr << "Failed to create adb control unit" << std::endl;
        return -1;
    }
#endif

#ifdef WITH_WIN32_CONTROLLER
    auto win32_type = MaaWin32ControllerType_Touch_SendMessage
                      | MaaWin32ControllerType_Key_SendMessage
                      | MaaWin32ControllerType_Screencap_GDI;
    std::cout << "********** Win32ControlUnitLibraryHolder::create_control_unit **********"
              << std::endl;
    auto win32_handle = MAA_NS::Win32ControlUnitLibraryHolder::create_control_unit(
        FindWindowA(nullptr, nullptr),
        win32_type,
        nullptr,
        nullptr);
    if (!win32_handle) {
        std::cerr << "Failed to create win32 control unit" << std::endl;
        return -1;
    }
#endif

#ifdef WITH_THRIFT_CONTROLLER
    std::cout << "********** ThriftControlUnitLibraryHolder::create_control_unit **********"
              << std::endl;
    auto thrift_handle = MAA_NS::ThriftControlUnitLibraryHolder::create_control_unit(
        MaaThriftControllerType_Socket,
        "127.0.0.1",
        8080,
        "{}");
    if (!thrift_handle) {
        std::cerr << "Failed to create thrift control unit" << std::endl;
        return -1;
    }
#endif

#ifdef WITH_DBG_CONTROLLER
    std::cout << "********** DbgControlUnitLibraryHolder::create_control_unit **********"
              << std::endl;
    auto dbg_handle = MAA_NS::DbgControlUnitLibraryHolder::create_control_unit(
        MaaDbgControllerType_CarouselImage,
        "./");
    if (!dbg_handle) {
        std::cerr << "Failed to create dbg control unit" << std::endl;
        return -1;
    }
#endif

    std::cout << "********** Done **********" << std::endl;
    return 0;
}