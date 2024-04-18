#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "MaaFramework/MaaDef.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

struct Executor
{
    std::string exec_path;
    std::vector<std::string> exec_param;

    MEO_JSONIZATION(exec_path, MEO_OPT exec_param);
};

struct InterfaceData
{
    struct Controller
    {
        struct AdbConfig
        {
            int32_t touch = MaaAdbControllerType_Touch_AutoDetect;
            int32_t key = MaaAdbControllerType_Key_AutoDetect;
            int32_t screencap = MaaAdbControllerType_Screencap_FastestWay;

            json::object config;

            MEO_JSONIZATION(MEO_OPT touch, MEO_OPT key, MEO_OPT screencap, MEO_OPT config);
        };

        struct Win32Config
        {
            inline static std::string kMethodFind = "Find";
            inline static std::string kMethodSearch = "Search";
            inline static std::string kMethodCursor = "Cursor";
            inline static std::string kMethodDesktop = "Desktop";
            inline static std::string kMethodForeground = "Foreground";

            std::string method;

            std::string class_name;  // required by "Find" and "Search"
            std::string window_name; // required by "Find" and "Search"

            int32_t touch = MaaWin32ControllerType_Touch_Seize;
            int32_t key = MaaWin32ControllerType_Key_Seize;
            int32_t screencap = MaaWin32ControllerType_Screencap_GDI;

            MEO_JSONIZATION(
                method,
                MEO_OPT class_name,
                MEO_OPT window_name,
                MEO_OPT touch,
                MEO_OPT key,
                MEO_OPT screencap);
        };

        inline static std::string kTypeAdb = "Adb";
        inline static std::string kTypeWin32 = "Win32";

        std::string name;
        std::string type; // "Adb", "Win32"

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
        json::object param;
        std::vector<std::string> option;

        MEO_JSONIZATION(name, entry, MEO_OPT param, MEO_OPT option);
    };

    struct Case
    {
        std::string name;
        json::object param;

        MEO_JSONIZATION(name, param);
    };

    std::vector<Controller> controller;
    std::vector<Resource> resource;
    std::vector<Task> task;
    std::unordered_map<std::string, std::vector<Case>> option;
    std::unordered_map<std::string, Executor> recognizer;
    std::unordered_map<std::string, Executor> action;
    std::string version;
    std::string message;

    MEO_JSONIZATION(
        controller,
        resource,
        task,
        MEO_OPT option,
        MEO_OPT recognizer,
        MEO_OPT action,
        MEO_OPT version,
        MEO_OPT message);
};

struct Configuration
{
    struct Controller
    {
        std::string name;
        std::string type;

        MEO_JSONIZATION(name, type);
    };

    struct Win32Config
    {
        MaaWin32Hwnd hwnd = nullptr;

        int _placeholder = 0;

        MEO_JSONIZATION(MEO_OPT _placeholder);
    };

    struct AdbConfig
    {
        std::string adb_path;
        std::string address;

        MEO_JSONIZATION(adb_path, address);
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

    MEO_JSONIZATION(controller, MEO_OPT adb, MEO_OPT win32, resource, task);
};

struct RuntimeParam
{
    struct AdbParam
    {
        std::string adb_path;
        std::string address;
        int32_t controller_type = 0;
        std::string config;
        std::string agent_path;
    };

    struct Win32Param
    {
        MaaWin32Hwnd hwnd = nullptr;
        int32_t controller_type = 0;
    };

    struct Task
    {
        std::string name;
        std::string entry;
        json::object param;
    };

    std::variant<AdbParam, Win32Param> controller_param;
    std::vector<std::string> resource_path;

    std::vector<Task> task;
    std::unordered_map<std::string, Executor> recognizer;
    std::unordered_map<std::string, Executor> action;
};

MAA_PROJECT_INTERFACE_NS_END
