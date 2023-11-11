#include "Utils/ControlUnitLibraryHolder.h"

MAA_CTRL_NS_BEGIN

template <typename ControlUnitT>
bool check_version(const std::string& func_name)
{
    using get_version_t = MaaStringView(void);
    auto version_func = ControlUnitT::template get_function<get_version_t>(func_name);
    if (!version_func) {
        LogError << "Failed to get function get_version";
        return false;
    }
    auto version = version_func();
    LogInfo << "Version:" << version;

    if (std::strcmp(version, MAA_VERSION) != 0) {
        LogWarn << "ControlUnit and MaaFramework are not same version,"
                << "ControlUnit:" << version << "MaaFramework:" << MAA_VERSION;
    }
    return true;
}

std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> AdbControlUnitLibraryHolder::create_control_unit(
    MaaStringView adb_path, MaaStringView adb_serial, MaaAdbControllerType type, MaaStringView config,
    MaaStringView agent_path, MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg)
{
    if (!load_library(libname_)) {
        LogError << "Failed to load library" << VAR(libname_);
        return nullptr;
    }

    check_version<AdbControlUnitLibraryHolder>(version_func_name_);

    using create_control_unit_t = MaaControlUnitHandle( //
        MaaStringView, MaaStringView, MaaAdbControllerType, MaaStringView, MaaStringView, MaaControllerCallback,
        MaaCallbackTransparentArg);

    using destroy_control_unit_t = void(MaaControlUnitHandle);

    auto create_control_unit_func = get_function<create_control_unit_t>(create_func_name_);
    if (!create_control_unit_func) {
        LogError << "Failed to get function create_control_unit";
        return nullptr;
    }

    auto destroy_control_unit_func = get_function<destroy_control_unit_t>(destroy_func_name_);
    if (!destroy_control_unit_func) {
        LogError << "Failed to get function destroy_control_unit";
        return nullptr;
    }

    auto control_unit_handle = create_control_unit_func( //
        adb_path, adb_serial, type, config, agent_path, callback, callback_arg);

    if (!control_unit_handle) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI>(control_unit_handle, destroy_control_unit_func);
}

std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> DbgControlUnitLibraryHolder::create_control_unit(
    MaaDbgControllerType type, MaaStringView read_path)
{
    if (!load_library(libname_)) {
        LogError << "Failed to load library" << VAR(libname_);
        return nullptr;
    }

    check_version<DbgControlUnitLibraryHolder>(version_func_name_);

    using create_control_unit_t = MaaControlUnitHandle(MaaDbgControllerType, MaaStringView);

    using destroy_control_unit_t = void(MaaControlUnitHandle);

    auto create_control_unit_func = get_function<create_control_unit_t>(create_func_name_);
    if (!create_control_unit_func) {
        LogError << "Failed to get function create_control_unit";
        return nullptr;
    }

    auto destroy_control_unit_func = get_function<destroy_control_unit_t>(destroy_func_name_);
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

std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> ThriftControlUnitLibraryHolder::create_control_unit(
    MaaThriftControllerType type, MaaStringView host, int32_t port, MaaStringView config)
{
    if (!load_library(libname_)) {
        LogError << "Failed to load library" << VAR(libname_);
        return nullptr;
    }

    check_version<ThriftControlUnitLibraryHolder>(version_func_name_);

    using create_control_unit_t = MaaControlUnitHandle(MaaThriftControllerType, MaaStringView, int32_t, MaaStringView);

    using destroy_control_unit_t = void(MaaControlUnitHandle);

    auto create_control_unit_func = get_function<create_control_unit_t>(create_func_name_);
    if (!create_control_unit_func) {
        LogError << "Failed to get function create_control_unit";
        return nullptr;
    }

    auto destroy_control_unit_func = get_function<destroy_control_unit_t>(destroy_func_name_);
    if (!destroy_control_unit_func) {
        LogError << "Failed to get function destroy_control_unit";
        return nullptr;
    }

    auto control_unit_handle = create_control_unit_func(type, host, port, config);

    if (!control_unit_handle) {
        LogError << "Failed to create control unit";
        return nullptr;
    }

    return std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI>(control_unit_handle, destroy_control_unit_func);
}

MAA_CTRL_NS_END
