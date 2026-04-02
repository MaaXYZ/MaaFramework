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
class MacOSControlUnitAPI;
class GamepadControlUnitAPI;
class CustomControlUnitAPI;
class WlRootsControlUnitAPI;
class FullControlUnitAPI;
class AndroidNativeControlUnitAPI;
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

class MacOSControlUnitLibraryHolder : public LibraryHolder<MacOSControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::MacOSControlUnitAPI>
        create_control_unit(uint32_t window_id, MaaMacOSScreencapMethod screencap_method, MaaMacOSInputMethod input_method);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaMacOSControlUnit");
    inline static const std::string version_func_name_ = "MaaMacOSControlUnitGetVersion";
    inline static const std::string create_func_name_ = "MaaMacOSControlUnitCreate";
    inline static const std::string destroy_func_name_ = "MaaMacOSControlUnitDestroy";
};

class AndroidNativeControlUnitLibraryHolder : public LibraryHolder<AndroidNativeControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::AndroidNativeControlUnitAPI> create_control_unit(const char* config_json);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaAndroidNativeControlUnit");
    inline static const std::string version_func_name_ = "MaaAndroidNativeControlUnitGetVersion";
    inline static const std::string create_func_name_ = "MaaAndroidNativeControlUnitCreate";
    inline static const std::string destroy_func_name_ = "MaaAndroidNativeControlUnitDestroy";
};

class DbgControlUnitLibraryHolder : public LibraryHolder<DbgControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> create_control_unit(const char* read_path);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaDbgControlUnit");
    inline static const std::string version_func_name_ = "MaaDbgControlUnitGetVersion";
    inline static const std::string create_func_name_ = "MaaDbgControlUnitCreate";
    inline static const std::string destroy_func_name_ = "MaaDbgControlUnitDestroy";
};

class ReplayControlUnitLibraryHolder : public LibraryHolder<ReplayControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::FullControlUnitAPI> create_control_unit(const char* recording_path);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaReplayControlUnit");
    inline static const std::string version_func_name_ = "MaaReplayControlUnitGetVersion";
    inline static const std::string create_func_name_ = "MaaReplayControlUnitCreate";
    inline static const std::string destroy_func_name_ = "MaaReplayControlUnitDestroy";
};

class CustomControlUnitLibraryHolder : public LibraryHolder<CustomControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::CustomControlUnitAPI>
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

class GamepadControlUnitLibraryHolder : public LibraryHolder<GamepadControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::GamepadControlUnitAPI>
        create_control_unit(void* hWnd, MaaGamepadType gamepad_type, MaaWin32ScreencapMethod screencap_method);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaGamepadControlUnit");
    inline static const std::string version_func_name_ = "MaaGamepadControlUnitGetVersion";
    inline static const std::string create_func_name_ = "MaaGamepadControlUnitCreate";
    inline static const std::string destroy_func_name_ = "MaaGamepadControlUnitDestroy";
};

class RecordControlUnitLibraryHolder : public LibraryHolder<RecordControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::FullControlUnitAPI>
        create_control_unit(std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> inner, const char* recording_path);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaRecordControlUnit");
    inline static const std::string version_func_name_ = "MaaRecordControlUnitGetVersion";
    inline static const std::string create_func_name_ = "MaaRecordControlUnitCreate";
    inline static const std::string destroy_func_name_ = "MaaRecordControlUnitDestroy";
};

class WlRootsControlUnitLibraryHolder : public LibraryHolder<WlRootsControlUnitLibraryHolder>
{
public:
    static std::shared_ptr<MAA_CTRL_UNIT_NS::ControlUnitAPI> create_control_unit(const char* wlr_socket_path);

private:
    inline static const std::filesystem::path libname_ = MAA_NS::path("MaaWlRootsControlUnit");
    inline static const std::string version_func_name_ = "MaaWlRootsControlUnitGetVersion";
    inline static const std::string create_func_name_ = "MaaWlRootsControlUnitCreate";
    inline static const std::string destroy_func_name_ = "MaaWlRootsControlUnitDestroy";
};

MAA_NS_END
