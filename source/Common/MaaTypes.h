#pragma once

#include "MaaConf.h"
#include "MaaDef.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

struct MaaInstanceSink
{
public:
    virtual void on_stop() {}
};

struct MaaResourceAPI : public MaaInstanceSink
{
public:
    virtual ~MaaResourceAPI() = default;

    virtual bool set_option(MaaResOption key, const std::string& value) = 0;

    virtual MaaResId post_resource(std::filesystem::path path) = 0;
    virtual MaaStatus status(MaaResId res_id) const = 0;
    virtual MaaStatus wait(MaaResId res_id) const = 0;
    virtual MaaBool loaded() const = 0;

    virtual std::string get_hash() const = 0;
};

struct MaaControllerAPI : public MaaInstanceSink
{
public:
    virtual ~MaaControllerAPI() = default;

    virtual bool set_option(MaaCtrlOption key, const std::string& value) = 0;

    virtual MaaCtrlId post_connection() = 0;
    virtual MaaCtrlId post_click(int x, int y) = 0;
    virtual MaaCtrlId post_swipe(std::vector<int> x_steps, std::vector<int> y_steps, std::vector<int> step_delay) = 0;
    virtual MaaCtrlId post_screencap() = 0;

    virtual MaaStatus status(MaaCtrlId ctrl_id) const = 0;
    virtual MaaStatus wait(MaaCtrlId ctrl_id) const = 0;
    virtual MaaBool connected() const = 0;

    virtual std::vector<uint8_t> get_image() const = 0;

    virtual std::string get_uuid() const = 0;
};

struct MaaInstanceAPI
{
public:
    virtual ~MaaInstanceAPI() = default;

    virtual bool bind_resource(MaaResourceAPI* resource) = 0;
    virtual bool bind_controller(MaaControllerAPI* controller) = 0;
    virtual bool inited() const = 0;

    virtual bool set_option(MaaInstOption key, const std::string& value) = 0;

    virtual MaaTaskId post_task(std::string task, std::string_view param) = 0;
    virtual bool set_task_param(MaaTaskId task_id, std::string_view param) = 0;

    virtual MaaStatus status(MaaTaskId task_id) const = 0;
    virtual MaaStatus wait(MaaTaskId task_id) const = 0;
    virtual MaaBool all_finished() const = 0;

    virtual void stop() = 0;

    virtual std::string get_resource_hash() const = 0;
    virtual std::string get_controller_uuid() const = 0;
};
