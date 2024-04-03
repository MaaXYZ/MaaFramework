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
        std::string name;
        std::string type; // "Adb", "Win32"
        int32_t touch = MaaAdbControllerType_Touch_AutoDetect;
        int32_t key = MaaAdbControllerType_Key_AutoDetect;
        int32_t screencap = MaaAdbControllerType_Screencap_FastestWay;
        json::object config;

        MEO_JSONIZATION(name, type, MEO_OPT touch, MEO_OPT key, MEO_OPT screencap, MEO_OPT config);
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

    struct Option
    {
        struct Case
        {
            std::string name;
            json::object param;

            MEO_JSONIZATION(name, param);
        };

        std::vector<Case> cases;
        std::string default_case; // case.name

        MEO_JSONIZATION(cases, MEO_OPT default_case);
    };

    std::vector<Controller> controller;
    std::vector<Resource> resource;
    std::vector<Task> task;
    std::unordered_map<std::string, Option> option;
    std::unordered_map<std::string, Executor> recognizer;
    std::unordered_map<std::string, Executor> action;
    std::string version;
    std::string message;

    MEO_JSONIZATION(
        MEO_OPT controller,
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
        std::string adb_path;
        std::string address;

        MEO_JSONIZATION(name, adb_path, address);
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
    std::string resource;
    std::vector<Task> task;

    MEO_JSONIZATION(controller, resource, task);
};

struct RuntimeParam
{
    struct AdbParam
    {
        std::string name;
        std::string adb_path;
        std::string address;
        int32_t controller_type = MaaAdbControllerType_Touch_AutoDetect
                                  | MaaAdbControllerType_Key_AutoDetect
                                  | MaaAdbControllerType_Screencap_FastestWay;
        std::string config = json::object().dumps();
        std::string agent_path;

        MEO_JSONIZATION(name, adb_path, address, controller_type, config, agent_path);
    };

    struct Task
    {
        std::string name;
        std::string entry;
        json::object param;

        MEO_JSONIZATION(name, entry, param);
    };

    AdbParam adb_param;
    std::vector<std::string> resource_path;

    std::vector<Task> task;
    std::unordered_map<std::string, Executor> recognizer;
    std::unordered_map<std::string, Executor> action;

    MEO_JSONIZATION(adb_param, resource_path, task, recognizer, action);
};

MAA_PROJECT_INTERFACE_NS_END
