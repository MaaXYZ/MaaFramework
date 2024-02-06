#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <meojson/json.hpp>

#include "Conf/Conf.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

struct Resource
{
    std::string name;
    std::vector<std::string> path;

    MEO_JSONIZATION(name, path);
};

struct Option
{
    struct Case
    {
        std::string name;
        json::value param;

        MEO_JSONIZATION(name, param);
    };

    std::string name;
    std::vector<Case> cases;
    std::string default_case; // case.name

    MEO_JSONIZATION(name, cases, MEO_OPT default_case);
};

struct Entry
{
    std::string name;
    json::value param;
    std::vector<Option> options;

    MEO_JSONIZATION(name, MEO_OPT param, MEO_OPT options);
};

struct Executor
{
    // enum class Type
    //{
    //     Action,
    //     Recognizer,
    // };
    // Type type = Type::Action;

    std::string type;
    std::string name;
    std::string exec_path;
    std::vector<std::string> exec_param;

    MEO_JSONIZATION(type, name, exec_path, MEO_OPT exec_param);
};

struct InterfaceData
{
    std::vector<Resource> resource;
    std::vector<Entry> entry;
    std::vector<Executor> executor;

    MEO_JSONIZATION(resource, entry, MEO_OPT executor);
};

struct Configuration
{
    struct Task
    {
        std::string name;
        std::unordered_map<std::string, std::string> option;

        MEO_JSONIZATION(name, MEO_OPT option);
    };

    std::string resource;
    std::vector<Task> task;

    MEO_JSONIZATION(resource, task);
};

struct RuntimeParam
{
    struct Task
    {
        std::string entry;
        json::value param;
    };
    std::vector<std::string> resource_path;
    std::vector<Task> task;
    std::vector<Executor> executor;
};

MAA_PROJECT_INTERFACE_NS_END
