#pragma once

#include <string>
#include <vector>

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
    std::string task;
    json::value param;
    std::vector<Option> options;

    MEO_JSONIZATION(name, task, MEO_OPT param, MEO_OPT options);
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

MAA_PROJECT_INTERFACE_NS_END
