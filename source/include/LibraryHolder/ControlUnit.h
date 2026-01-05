#pragma once

#include <memory>

#include "MaaFramework/Instance/MaaCustomController.h"
#include "MaaUtils/LibraryHolder.h"
#include "MaaUtils/Platform.h"

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN
class ControlUnitAPI;
class AdbControlUnitAPI;
class Win32ControlUnitAPI;
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
    static std::shared_ptr<MAA_CTRL_UNIT_NS::Win32ControlUnitAPI> create_control_unit(
        void* hWnd,
        MaaWin32ScreencapMethod screencap_method,
        MaaWin32InputMethod mouse_method,
        MaaWin32InputMethod keyboard_method);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaWin32ControlUnit");
    inline static const std::string version_func_name_ = "MaaWin32ControlUnitGetVersion";
    inline static const std::string create_func_name_ = "MaaWin32ControlUnitCreate";
    inline static const std::string destroy_func_name_ = "MaaWin32ControlUnitDestroy";
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

class PlayCoverControlUnitLibraryHolder : public LibraryHolder<PlayCoverControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> create_control_unit(const char* address, const char* uuid);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaPlayCoverControlUnit");
    inline static const std::string version_func_name_ = "MaaPlayCoverControlUnitGetVersion";
    inline static const std::string create_func_name_ = "MaaPlayCoverControlUnitCreate";
    inline static const std::string destroy_func_name_ = "MaaPlayCoverControlUnitDestroy";
};

MAA_NS_END
