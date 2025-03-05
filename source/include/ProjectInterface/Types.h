#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "MaaFramework/MaaDef.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

struct InterfaceData
{
    struct Controller
    {
        struct AdbConfig
        {
            MaaAdbScreencapMethod screencap = MaaAdbScreencapMethod_Default;
            MaaAdbInputMethod input = MaaAdbInputMethod_Default;

            json::object config;

            MEO_JSONIZATION(MEO_OPT screencap, MEO_OPT input, MEO_OPT config);
        };

        struct Win32Config
        {
            std::string class_regex;
            std::string window_regex;

            MaaWin32ScreencapMethod screencap = MaaWin32ScreencapMethod_DXGI_DesktopDup;
            MaaWin32InputMethod input = MaaWin32InputMethod_Seize;

            MEO_JSONIZATION(MEO_OPT class_regex, MEO_OPT window_regex, MEO_OPT screencap, MEO_OPT input);
        };

        inline static std::string kTypeAdb = "Adb";
        inline static std::string kTypeWin32 = "Win32";

        std::string name;
        std::string type; // "Adb", "Win32"
        enum class Type
        {
            Invalid,
            Adb,
            Win32,
        } type_enum = Type::Invalid;

        AdbConfig adb;
        Win32Config win32;

        MEO_JSONIZATION(name, type, MEO_OPT adb, MEO_OPT win32);
    };

    struct Resource
    {
        std::string name;
        std::vector<std::string> path;

        MEO_JSONIZATION(name, path);
    };

    struct Task
    {
        std::string name;
        std::string entry;
        json::object pipeline_override;
        std::vector<std::string> option;

        MEO_JSONIZATION(name, entry, MEO_OPT pipeline_override, MEO_OPT option);
    };

    struct Option
    {
        struct Case
        {
            std::string name;
            json::object pipeline_override;

            MEO_JSONIZATION(name, pipeline_override);
        };

        std::vector<Case> cases;
        std::string default_case; // case.name

        MEO_JSONIZATION(cases, MEO_OPT default_case);
    };

    struct Agent
    {
        std::string child_exec;
        std::vector<std::string> child_args;
        std::string identifier;

        MEO_JSONIZATION(child_exec, MEO_OPT child_args, MEO_OPT identifier);
    };

    std::vector<Controller> controller;
    std::vector<Resource> resource;
    std::vector<Task> task;
    std::unordered_map<std::string, Option> option;
    std::string version;
    std::string message;
    Agent agent;

    MEO_JSONIZATION(controller, resource, task, MEO_OPT option, MEO_OPT version, MEO_OPT message, MEO_OPT agent);
};

struct Configuration
{
    struct Controller
    {
        std::string name;
        InterfaceData::Controller::Type type_enum = InterfaceData::Controller::Type::Invalid;

        MEO_JSONIZATION(name);
    };

    struct Win32Config
    {
        void* hwnd = nullptr;
        std::wstring class_name;
        std::wstring window_name;

        int _placeholder = 0;

        MEO_JSONIZATION(MEO_OPT _placeholder);
    };

    struct AdbConfig
    {
        std::string adb_path;
        std::string address;
        json::object config;

        MEO_JSONIZATION(adb_path, address, config);
    };

    struct Option
    {
        std::string name;
        std::string value;

        MEO_JSONIZATION(name, value);
    };

    struct Task
    {
        std::string name;
        std::vector<Option> option;

        MEO_JSONIZATION(name, MEO_OPT option);
    };

    Controller controller;
    AdbConfig adb;
    Win32Config win32;
    std::string resource;
    std::vector<Task> task;
    int32_t gpu = MaaInferenceDevice_Auto;

    MEO_JSONIZATION(controller, MEO_OPT adb, MEO_OPT win32, resource, task, MEO_OPT gpu);
};

struct RuntimeParam
{
    struct AdbParam
    {
        std::string adb_path;
        std::string address;
        MaaAdbScreencapMethod screencap = MaaAdbScreencapMethod_None;
        MaaAdbInputMethod input = MaaAdbInputMethod_None;
        std::string config;
        std::string agent_path;
    };

    struct Win32Param
    {
        void* hwnd = nullptr;
        MaaWin32ScreencapMethod screencap = MaaWin32ScreencapMethod_None;
        MaaWin32InputMethod input = MaaWin32InputMethod_None;
    };

    struct Task
    {
        std::string name;
        std::string entry;
        json::object pipeline_override;
    };

    struct Agent
    {
        std::filesystem::path child_exec;
        std::vector<std::string> child_args;
        std::string identifier;
    };

    std::variant<std::monostate, AdbParam, Win32Param> controller_param;
    std::vector<std::filesystem::path> resource_path;

    std::vector<Task> task;
    int32_t gpu = MaaInferenceDevice_Auto;

    std::optional<Agent> agent;
};

struct CustomRecognitionSession
{
    MaaCustomRecognitionCallback recognition = nullptr;
    void* trans_arg = nullptr;
};

struct CustomActionSession
{
    MaaCustomActionCallback action = nullptr;
    void* trans_arg = nullptr;
};

MAA_PROJECT_INTERFACE_NS_END
