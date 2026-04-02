#include "LibraryHolder/ControlUnit.h"

#include <filesystem>

#include "MaaControlUnit/AdbControlUnitAPI.h"
#include "MaaControlUnit/AndroidNativeControlUnitAPI.h"
#include "MaaControlUnit/CustomControlUnitAPI.h"
#include "MaaControlUnit/DbgControlUnitAPI.h"
#include "MaaControlUnit/GamepadControlUnitAPI.h"
#include "MaaControlUnit/MacOSControlUnitAPI.h"
#include "MaaControlUnit/PlayCoverControlUnitAPI.h"
#include "MaaControlUnit/RecordControlUnitAPI.h"
#include "MaaControlUnit/ReplayControlUnitAPI.h"
#include "MaaControlUnit/Win32ControlUnitAPI.h"
#include "MaaControlUnit/WlRootsControlUnitAPI.h"
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

std::shared_ptr<MAA_CTRL_UNIT_NS::AndroidNativeControlUnitAPI>
    AndroidNativeControlUnitLibraryHolder::create_control_unit(const char* config_json)
{
    if (!load_library(library_dir() / libname_)) {
        LogError << "Failed to load library" << VAR(library_dir()) << VAR(libname_);
        return nullptr;
    }

    check_version<AndroidNativeControlUnitLibraryHolder, decltype(MaaAndroidNativeControlUnitGetVersion)>(version_func_name_);

    auto create_control_unit_func = get_function<decltype(MaaAndroidNativeControlUnitCreate)>(create_func_name_);
    if (!create_control_unit_func) {
        LogError << "Failed to get function create_control_unit";
        return nullptr;
    }

    auto destroy_control_unit_func = get_function<decltype(MaaAndroidNativeControlUnitDestroy)>(destroy_func_name_);
    if (!destroy_control_unit_func) {
        LogError << "Failed to get function destroy_control_unit";
        return nullptr;
    }

    auto control_unit_handle = create_control_unit_func(config_json);
    if (!control_unit_handle) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return std::shared_ptr<MAA_CTRL_UNIT_NS::AndroidNativeControlUnitAPI>(control_unit_handle, destroy_control_unit_func);
}

std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> DbgControlUnitLibraryHolder::create_control_unit(const char* read_path)
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

    auto control_unit_handle = create_control_unit_func(read_path);

    if (!control_unit_handle) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI>(control_unit_handle, destroy_control_unit_func);
}

std::shared_ptr<MAA_CTRL_UNIT_NS::FullControlUnitAPI> ReplayControlUnitLibraryHolder::create_control_unit(const char* recording_path)
{
    if (!load_library(library_dir() / libname_)) {
        LogError << "Failed to load library" << VAR(library_dir()) << VAR(libname_);
        return nullptr;
    }

    check_version<ReplayControlUnitLibraryHolder, decltype(MaaReplayControlUnitGetVersion)>(version_func_name_);

    auto create_control_unit_func = get_function<decltype(MaaReplayControlUnitCreate)>(create_func_name_);
    if (!create_control_unit_func) {
        LogError << "Failed to get function create_control_unit";
        return nullptr;
    }

    auto destroy_control_unit_func = get_function<decltype(MaaReplayControlUnitDestroy)>(destroy_func_name_);
    if (!destroy_control_unit_func) {
        LogError << "Failed to get function destroy_control_unit";
        return nullptr;
    }

    auto control_unit_handle = create_control_unit_func(recording_path);

    if (!control_unit_handle) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return std::shared_ptr<MAA_CTRL_UNIT_NS::FullControlUnitAPI>(control_unit_handle, destroy_control_unit_func);
}

std::shared_ptr<MAA_CTRL_UNIT_NS::CustomControlUnitAPI>
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

    return std::shared_ptr<MAA_CTRL_UNIT_NS::CustomControlUnitAPI>(control_unit_handle, destroy_control_unit_func);
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

std::shared_ptr<MAA_CTRL_UNIT_NS::GamepadControlUnitAPI>
    GamepadControlUnitLibraryHolder::create_control_unit(void* hWnd, MaaGamepadType gamepad_type, MaaWin32ScreencapMethod screencap_method)
{
    if (!load_library(library_dir() / libname_)) {
        LogError << "Failed to load library" << VAR(library_dir()) << VAR(libname_);
        return nullptr;
    }

    check_version<GamepadControlUnitLibraryHolder, decltype(MaaGamepadControlUnitGetVersion)>(version_func_name_);

    auto create_control_unit_func = get_function<decltype(MaaGamepadControlUnitCreate)>(create_func_name_);
    if (!create_control_unit_func) {
        LogError << "Failed to get function create_control_unit";
        return nullptr;
    }

    auto destroy_control_unit_func = get_function<decltype(MaaGamepadControlUnitDestroy)>(destroy_func_name_);
    if (!destroy_control_unit_func) {
        LogError << "Failed to get function destroy_control_unit";
        return nullptr;
    }

    auto control_unit_handle = create_control_unit_func(hWnd, gamepad_type, screencap_method);

    if (!control_unit_handle) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return std::shared_ptr<MAA_CTRL_UNIT_NS::GamepadControlUnitAPI>(control_unit_handle, destroy_control_unit_func);
}

std::shared_ptr<MAA_CTRL_UNIT_NS::FullControlUnitAPI>
    RecordControlUnitLibraryHolder::create_control_unit(std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> inner, const char* recording_path)
{
    if (!load_library(library_dir() / libname_)) {
        LogError << "Failed to load library" << VAR(library_dir()) << VAR(libname_);
        return nullptr;
    }

    check_version<RecordControlUnitLibraryHolder, decltype(MaaRecordControlUnitGetVersion)>(version_func_name_);

    auto create_control_unit_func = get_function<decltype(MaaRecordControlUnitCreate)>(create_func_name_);
    if (!create_control_unit_func) {
        LogError << "Failed to get function create_control_unit";
        return nullptr;
    }

    auto destroy_control_unit_func = get_function<decltype(MaaRecordControlUnitDestroy)>(destroy_func_name_);
    if (!destroy_control_unit_func) {
        LogError << "Failed to get function destroy_control_unit";
        return nullptr;
    }

    auto control_unit_handle = create_control_unit_func(&inner, recording_path);

    if (!control_unit_handle) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return std::shared_ptr<MAA_CTRL_UNIT_NS::FullControlUnitAPI>(control_unit_handle, destroy_control_unit_func);
}

std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> WlRootsControlUnitLibraryHolder::create_control_unit(const char* wlr_socket_path)
{
    if (!load_library(library_dir() / libname_)) {
        LogError << "Failed to load library" << VAR(library_dir()) << VAR(libname_);
        return nullptr;
    }

    check_version<WlRootsControlUnitLibraryHolder, decltype(MaaWlRootsControlUnitGetVersion)>(version_func_name_);

    auto create_control_unit_func = get_function<decltype(MaaWlRootsControlUnitCreate)>(create_func_name_);
    if (!create_control_unit_func) {
        LogError << "Failed to get function create_control_unit";
        return nullptr;
    }

    auto destroy_control_unit_func = get_function<decltype(MaaWlRootsControlUnitDestroy)>(destroy_func_name_);
    if (!destroy_control_unit_func) {
        LogError << "Failed to get function destroy_control_unit";
        return nullptr;
    }

    auto control_unit_handle = create_control_unit_func(wlr_socket_path);

    if (!control_unit_handle) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI>(control_unit_handle, destroy_control_unit_func);
}

std::shared_ptr<MAA_CTRL_UNIT_NS::MacOSControlUnitAPI> MacOSControlUnitLibraryHolder::create_control_unit(
    uint32_t window_id,
    MaaMacOSScreencapMethod screencap_method,
    MaaMacOSInputMethod input_method)
{
    if (!load_library(library_dir() / libname_)) {
        LogError << "Failed to load library" << VAR(library_dir()) << VAR(libname_);
        return nullptr;
    }

    check_version<MacOSControlUnitLibraryHolder, decltype(MaaMacOSControlUnitGetVersion)>(version_func_name_);

    auto create_control_unit_func = get_function<decltype(MaaMacOSControlUnitCreate)>(create_func_name_);
    if (!create_control_unit_func) {
        LogError << "Failed to get function create_control_unit";
        return nullptr;
    }

    auto destroy_control_unit_func = get_function<decltype(MaaMacOSControlUnitDestroy)>(destroy_func_name_);
    if (!destroy_control_unit_func) {
        LogError << "Failed to get function destroy_control_unit";
        return nullptr;
    }

    auto control_unit_handle = create_control_unit_func(window_id, screencap_method, input_method);

    if (!control_unit_handle) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return std::shared_ptr<MAA_CTRL_UNIT_NS::MacOSControlUnitAPI>(control_unit_handle, destroy_control_unit_func);
}

MAA_NS_END
