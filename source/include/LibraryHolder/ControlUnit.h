#pragma once

#include <MaaFramework/MaaAPI.h>
#include <memory>

#include "LibraryHolder.h"
#include "MaaFramework/Instance/MaaCustomController.h"
#include "Utils/Platform.h"

MAA_CTRL_UNIT_NS_BEGIN
class ControlUnitAPI;
class AdbControlUnitAPI;
class Win32ControlUnitAPI;
class MacControlUnitAPI;
class CustomControlUnitAPI;
MAA_CTRL_UNIT_NS_END

MAA_NS_BEGIN

class AdbControlUnitLibraryHolder : public LibraryHolder<AdbControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::AdbControlUnitAPI> create_control_unit(
        const char* adb_path,
        const char* adb_serial,
        MaaAdbScreencapMethod screencap_methods,
        MaaAdbInputMethod input_methods,
        const char* config,
        const char* agent_path);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaAdbControlUnit");
    inline static const std::string version_func_name_ = "MaaAdbControlUnitGetVersion";
    inline static const std::string create_func_name_ = "MaaAdbControlUnitCreate";
    inline static const std::string destroy_func_name_ = "MaaAdbControlUnitDestroy";
};

class Win32ControlUnitLibraryHolder : public LibraryHolder<Win32ControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::Win32ControlUnitAPI>
        create_control_unit(void* hWnd, MaaWin32ScreencapMethod screencap_method, MaaWin32InputMethod input_method);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaWin32ControlUnit");
    inline static const std::string version_func_name_ = "MaaWin32ControlUnitGetVersion";
    inline static const std::string create_func_name_ = "MaaWin32ControlUnitCreate";
    inline static const std::string destroy_func_name_ = "MaaWin32ControlUnitDestroy";
};

class MacControlUnitLibraryHolder : public LibraryHolder<MacControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI>
        create_control_unit(uint32_t windowId, MaaMacScreencapMethod screencap_method, MaaMacInputMethod input_method);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaMacControlUnit");
    inline static const std::string version_func_name_ = "MaaMacControlUnitGetVersion";
    inline static const std::string create_func_name_ = "MaaMacControlUnitCreate";
    inline static const std::string destroy_func_name_ = "MaaMacControlUnitDestroy";
};

class DbgControlUnitLibraryHolder : public LibraryHolder<DbgControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> create_control_unit(MaaDbgControllerType type, const char* read_path);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaDbgControlUnit");
    inline static const std::string version_func_name_ = "MaaDbgControlUnitGetVersion";
    inline static const std::string create_func_name_ = "MaaDbgControlUnitCreate";
    inline static const std::string destroy_func_name_ = "MaaDbgControlUnitDestroy";
};

class CustomControlUnitLibraryHolder : public LibraryHolder<CustomControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI>
        create_control_unit(MaaCustomControllerCallbacks* controller, void* controller_arg);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaCustomControlUnit");
    inline static const std::string version_func_name_ = "MaaCustomControlUnitGetVersion";
    inline static const std::string create_func_name_ = "MaaCustomControlUnitCreate";
    inline static const std::string destroy_func_name_ = "MaaCustomControlUnitDestroy";
};

MAA_NS_END
