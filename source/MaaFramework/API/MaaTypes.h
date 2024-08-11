#pragma once

#include "MaaFramework/MaaDef.h"

struct MaaResource
{
public:
    virtual ~MaaResource() = default;

    virtual bool set_option(MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size) = 0;

    virtual MaaResId post_path(std::filesystem::path path) = 0;

    virtual MaaStatus status(MaaResId res_id) const = 0;
    virtual MaaStatus wait(MaaResId res_id) const = 0;
    virtual MaaBool valid() const = 0;
    virtual MaaBool running() const = 0;
    virtual MaaBool clear() = 0;

    virtual std::string get_hash() const = 0;
    virtual std::vector<std::string> get_task_list() const = 0;
};

struct MaaController
{
public:
    virtual ~MaaController() = default;

    virtual bool set_option(MaaCtrlOption key, MaaOptionValue value, MaaOptionValueSize val_size) = 0;

    virtual MaaCtrlId post_connection() = 0;
    virtual MaaCtrlId post_click(int x, int y) = 0;
    virtual MaaCtrlId post_swipe(int x1, int y1, int x2, int y2, int duration) = 0;
    virtual MaaCtrlId post_press_key(int keycode) = 0;
    virtual MaaCtrlId post_input_text(std::string_view text) = 0;
    virtual MaaCtrlId post_start_app(std::string_view intent) = 0;
    virtual MaaCtrlId post_stop_app(std::string_view intent) = 0;
    virtual MaaCtrlId post_screencap() = 0;

    virtual MaaCtrlId post_touch_down(int contact, int x, int y, int pressure) = 0;
    virtual MaaCtrlId post_touch_move(int contact, int x, int y, int pressure) = 0;
    virtual MaaCtrlId post_touch_up(int contact) = 0;

    virtual MaaStatus status(MaaCtrlId ctrl_id) const = 0;
    virtual MaaStatus wait(MaaCtrlId ctrl_id) const = 0;
    virtual MaaBool connected() const = 0;
    virtual MaaBool running() const = 0;

    virtual cv::Mat cached_image() = 0;
    virtual std::string get_uuid() = 0;
};

struct MaaScheduler
{
public:
    virtual ~MaaScheduler() = default;

    virtual bool bind_resource(MaaResource* resource) = 0;
    virtual bool bind_controller(MaaController* controller) = 0;
    virtual bool inited() const = 0;

    virtual bool set_option(MaaSchedOption key, MaaOptionValue value, MaaOptionValueSize val_size) = 0;

    virtual MaaTaskId post_pipeline(std::string entry, std::string_view param) = 0;
    virtual MaaTaskId post_recognition(std::string entry, std::string_view param) = 0;
    virtual MaaTaskId post_action(std::string entry, std::string_view param) = 0;
    virtual bool set_task_param(MaaTaskId task_id, std::string_view param) = 0;

    virtual MaaStatus task_status(MaaTaskId task_id) const = 0;
    virtual MaaStatus task_wait(MaaTaskId task_id) const = 0;

    virtual MaaBool running() const = 0;
    virtual void post_stop() = 0;

    virtual MaaResource* resource() = 0;
    virtual MaaController* controller() = 0;
};

struct MaaSink
{
public:
    virtual ~MaaSink() = default;

    virtual void post_stop() {}
};
