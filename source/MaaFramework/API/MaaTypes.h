#pragma once

#include "Conf/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "Utils/NoWarningCVMat.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

struct MaaInstanceSink
{
public:
    virtual ~MaaInstanceSink() = default;
    virtual void on_stop() {}
};

struct MaaResourceAPI : public MaaInstanceSink
{
public:
    virtual ~MaaResourceAPI() = default;

    virtual bool set_option(MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size) = 0;

    virtual MaaResId post_path(std::filesystem::path path) = 0;
    virtual MaaStatus status(MaaResId res_id) const = 0;
    virtual MaaStatus wait(MaaResId res_id) const = 0;
    virtual MaaBool loaded() const = 0;

    virtual std::string get_hash() const = 0;
};

struct MaaControllerAPI : public MaaInstanceSink
{
public:
    virtual ~MaaControllerAPI() = default;

    virtual bool set_option(MaaCtrlOption key, MaaOptionValue value, MaaOptionValueSize val_size) = 0;

    virtual MaaCtrlId post_connection() = 0;
    virtual MaaCtrlId post_click(int x, int y) = 0;
    virtual MaaCtrlId post_swipe(std::vector<int> x_steps, std::vector<int> y_steps, std::vector<int> step_delay) = 0;
    virtual MaaCtrlId post_screencap() = 0;

    virtual MaaStatus status(MaaCtrlId ctrl_id) const = 0;
    virtual MaaStatus wait(MaaCtrlId ctrl_id) const = 0;
    virtual MaaBool connected() const = 0;

    virtual cv::Mat get_image() const = 0;
    virtual std::string get_uuid() const = 0;
};

struct MaaInstanceAPI
{
public:
    virtual ~MaaInstanceAPI() = default;

    virtual bool bind_resource(MaaResourceAPI* resource) = 0;
    virtual bool bind_controller(MaaControllerAPI* controller) = 0;
    virtual bool inited() const = 0;

    virtual bool set_option(MaaInstOption key, MaaOptionValue value, MaaOptionValueSize val_size) = 0;

    virtual MaaTaskId post_task(std::string entry, std::string_view param) = 0;
    virtual bool set_task_param(MaaTaskId task_id, std::string_view param) = 0;

    virtual bool register_custom_recognizer(std::string name, MaaCustomRecognizerHandle handle) = 0;
    virtual bool unregister_custom_recognizer(std::string name) = 0;
    virtual void clear_custom_recognizer() = 0;
    virtual bool register_custom_action(std::string name, MaaCustomActionHandle handle) = 0;
    virtual bool unregister_custom_action(std::string name) = 0;
    virtual void clear_custom_action() = 0;

    virtual MaaStatus status(MaaTaskId task_id) const = 0;
    virtual MaaStatus wait(MaaTaskId task_id) const = 0;
    virtual MaaBool all_finished() const = 0;

    virtual void stop() = 0;

    virtual MaaResourceHandle resource() = 0;
    virtual MaaControllerHandle controller() = 0;
};

struct MaaSyncContextAPI
{
public:
    virtual ~MaaSyncContextAPI() = default;

    virtual bool run_task(std::string task, std::string_view param) = 0;
    virtual bool run_recognizer(cv::Mat image, std::string task, std::string_view param,
                                /*out*/ cv::Rect& box, /*out*/ std::string& detail) = 0;
    virtual bool run_action(std::string task, std::string_view param, cv::Rect cur_box, std::string cur_detail) = 0;

    virtual void click(int x, int y) = 0;
    virtual void swipe(std::vector<int> x_steps, std::vector<int> y_steps, std::vector<int> step_delay) = 0;
    virtual cv::Mat screencap() = 0;

    virtual std::string task_result(const std::string& task_name) const = 0;

    virtual MaaInstanceHandle instance() = 0;
    virtual MaaResourceHandle resource() = 0;
    virtual MaaControllerHandle controller() = 0;
};

struct MaaStringBuffer
{
public:
    virtual ~MaaStringBuffer() = default;

    virtual const char* data() const = 0;
    virtual size_t size() const = 0;

    virtual const std::string& get() const = 0;

    virtual void set(std::string str) = 0;
};

struct MaaImageBuffer
{
public:
    virtual ~MaaImageBuffer() = default;

    virtual void* raw_data() const = 0;
    virtual int32_t width() const = 0;
    virtual int32_t height() const = 0;
    virtual int32_t type() const = 0;

    virtual uint8_t* encoded() = 0;
    virtual size_t encoded_size() = 0;

    virtual const cv::Mat& get() const = 0;

    virtual void set(cv::Mat image) = 0;
};
