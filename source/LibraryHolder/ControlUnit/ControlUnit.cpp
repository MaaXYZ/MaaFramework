#include "LibraryHolder/ControlUnit.h"

#include <filesystem>

#include "ControlUnit/AdbControlUnitAPI.h"
#include "ControlUnit/CustomControlUnitAPI.h"
#include "ControlUnit/DbgControlUnitAPI.h"
#include "ControlUnit/PlayCoverControlUnitAPI.h"
#include "ControlUnit/Win32ControlUnitAPI.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Runtime.h"

MAA_NS_BEGIN

template <typename ControlUnitT, typename GetVersionT>
bool check_version(const std::string& func_name)
{
    auto version_func = ControlUnitT::template get_function<GetVersionT>(func_name);
    if (!version_func) {
        LogError << "Failed to get function get_version";
        return false;
    }
    auto version = version_func();
    LogInfo << typeid(ControlUnitT).name() << "Library version:" << version;

    if (std::strcmp(version, MAA_VERSION) != 0) {
        LogWarn << "ControlUnit and MaaFramework are not same version," << "ControlUnit:" << version << "MaaFramework:" << MAA_VERSION;
    }
    return true;
}

std::shared_ptr<MAA_CTRL_UNIT_NS::AdbControlUnitAPI> AdbControlUnitLibraryHolder::create_control_unit(
    const char* adb_path,
    const char* adb_serial,
    MaaAdbScreencapMethod screencap_methods,
    MaaAdbInputMethod input_methods,
    const char* config,
    const char* agent_path)
{
    if (!load_library(library_dir() / libname_)) {
        LogError << "Failed to load library" << VAR(library_dir()) << VAR(libname_);
        return nullptr;
    }

    check_version<AdbControlUnitLibraryHolder, decltype(MaaAdbControlUnitGetVersion)>(version_func_name_);

    auto create_control_unit_func = get_function<decltype(MaaAdbControlUnitCreate)>(create_func_name_);
    if (!create_control_unit_func) {
        LogError << "Failed to get function create_control_unit";
        return nullptr;
    }

    auto destroy_control_unit_func = get_function<decltype(MaaAdbControlUnitDestroy)>(destroy_func_name_);
    if (!destroy_control_unit_func) {
        LogError << "Failed to get function destroy_control_unit";
        return nullptr;
    }

    auto control_unit_handle = create_control_unit_func(adb_path, adb_serial, screencap_methods, input_methods, config, agent_path);

    if (!control_unit_handle) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return std::shared_ptr<MAA_CTRL_UNIT_NS::AdbControlUnitAPI>(control_unit_handle, destroy_control_unit_func);
}

std::shared_ptr<MAA_CTRL_UNIT_NS::Win32ControlUnitAPI> Win32ControlUnitLibraryHolder::create_control_unit(
    void* hWnd,
    MaaWin32ScreencapMethod screencap_method,
    MaaWin32InputMethod mouse_method,
    MaaWin32InputMethod keyboard_method)
{
    if (!load_library(library_dir() / libname_)) {
        LogError << "Failed to load library" << VAR(library_dir()) << VAR(libname_);
        return nullptr;
    }

    check_version<Win32ControlUnitLibraryHolder, decltype(MaaWin32ControlUnitGetVersion)>(version_func_name_);

    auto create_control_unit_func = get_function<decltype(MaaWin32ControlUnitCreate)>(create_func_name_);
    if (!create_control_unit_func) {
        LogError << "Failed to get function create_control_unit";
        return nullptr;
    }

    auto destroy_control_unit_func = get_function<decltype(MaaWin32ControlUnitDestroy)>(destroy_func_name_);
    if (!destroy_control_unit_func) {
        LogError << "Failed to get function destroy_control_unit";
        return nullptr;
    }

    auto control_unit_handle = create_control_unit_func(hWnd, screencap_method, mouse_method, keyboard_method);

    if (!control_unit_handle) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return std::shared_ptr<MAA_CTRL_UNIT_NS::Win32ControlUnitAPI>(control_unit_handle, destroy_control_unit_func);
}

std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI>
    DbgControlUnitLibraryHolder::create_control_unit(MaaDbgControllerType type, const char* read_path)
{
    if (!load_library(library_dir() / libname_)) {
        LogError << "Failed to load library" << VAR(library_dir()) << VAR(libname_);
        return nullptr;
    }

    check_version<DbgControlUnitLibraryHolder, decltype(MaaDbgControlUnitGetVersion)>(version_func_name_);

    auto create_control_unit_func = get_function<decltype(MaaDbgControlUnitCreate)>(create_func_name_);
    if (!create_control_unit_func) {
        LogError << "Failed to get function create_control_unit";
        return nullptr;
    }

    auto destroy_control_unit_func = get_function<decltype(MaaDbgControlUnitDestroy)>(destroy_func_name_);
    if (!destroy_control_unit_func) {
        LogError << "Failed to get function destroy_control_unit";
        return nullptr;
    }

    auto control_unit_handle = create_control_unit_func(type, read_path);

    if (!control_unit_handle) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI>(control_unit_handle, destroy_control_unit_func);
}

std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI>
    CustomControlUnitLibraryHolder::create_control_unit(MaaCustomControllerCallbacks* controller, void* controller_arg)
{
    if (!load_library(library_dir() / libname_)) {
        LogError << "Failed to load library" << VAR(library_dir()) << VAR(libname_);
        return nullptr;
    }

    check_version<CustomControlUnitLibraryHolder, decltype(MaaCustomControlUnitGetVersion)>(version_func_name_);

    auto create_control_unit_func = get_function<decltype(MaaCustomControlUnitCreate)>(create_func_name_);
    if (!create_control_unit_func) {
        LogError << "Failed to get function create_control_unit";
        return nullptr;
    }

    auto destroy_control_unit_func = get_function<decltype(MaaCustomControlUnitDestroy)>(destroy_func_name_);
    if (!destroy_control_unit_func) {
        LogError << "Failed to get function destroy_control_unit";
        return nullptr;
    }

    auto control_unit_handle = create_control_unit_func(controller, controller_arg);

    if (!control_unit_handle) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI>(control_unit_handle, destroy_control_unit_func);
}

std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI>
    PlayCoverControlUnitLibraryHolder::create_control_unit(const char* address, const char* uuid)
{
    if (!load_library(library_dir() / libname_)) {
        LogError << "Failed to load library" << VAR(library_dir()) << VAR(libname_);
        return nullptr;
    }

    check_version<PlayCoverControlUnitLibraryHolder, decltype(MaaPlayCoverControlUnitGetVersion)>(version_func_name_);

    auto create_control_unit_func = get_function<decltype(MaaPlayCoverControlUnitCreate)>(create_func_name_);
    if (!create_control_unit_func) {
        LogError << "Failed to get function create_control_unit";
        return nullptr;
    }

    auto destroy_control_unit_func = get_function<decltype(MaaPlayCoverControlUnitDestroy)>(destroy_func_name_);
    if (!destroy_control_unit_func) {
        LogError << "Failed to get function destroy_control_unit";
        return nullptr;
    }

    auto control_unit_handle = create_control_unit_func(address, uuid);

    if (!control_unit_handle) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI>(control_unit_handle, destroy_control_unit_func);
}

MAA_NS_END
