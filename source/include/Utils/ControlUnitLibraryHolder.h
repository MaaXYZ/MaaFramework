#pragma once

#include "Utils/LibraryHolder.h"

#include <memory>

#include "ControlUnit/ControlUnitAPI.h"
#include "Utils/Platform.h"

MAA_CTRL_NS_BEGIN

class MAA_UTILS_API AdbControlUnitLibraryHolder : public LibraryHolder<AdbControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> create_control_unit( //
        MaaStringView adb_path, MaaStringView adb_serial, MaaAdbControllerType type, MaaStringView config,
        MaaStringView agent_path, MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaAdbControlUnit");
    inline static const std::string version_func_name_ = "get_version";
    inline static const std::string create_func_name_ = "create_control_unit";
    inline static const std::string destroy_func_name_ = "destroy_control_unit";
};

class MAA_UTILS_API DbgControlUnitLibraryHolder : public LibraryHolder<DbgControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> create_control_unit( //
        MaaDbgControllerType type, MaaStringView read_path);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaDbgControlUnit");
    inline static const std::string version_func_name_ = "get_version";
    inline static const std::string create_func_name_ = "create_control_unit";
    inline static const std::string destroy_func_name_ = "destroy_control_unit";
};

class MAA_UTILS_API ThriftControlUnitLibraryHolder : public LibraryHolder<ThriftControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> create_control_unit( //
        MaaThriftControllerType type, MaaStringView host, int32_t port, MaaStringView config);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaThriftControlUnit");
    inline static const std::string version_func_name_ = "get_version";
    inline static const std::string create_func_name_ = "create_control_unit";
    inline static const std::string destroy_func_name_ = "destroy_control_unit";
};

MAA_CTRL_NS_END
