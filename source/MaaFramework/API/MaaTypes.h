#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <meojson/json.hpp>

#include "MaaFramework/MaaDef.h"
#include "Task/TaskResultTypes.h"
#include "Utils/NoWarningCVMat.hpp"

struct MaaResource
{
public:
    virtual ~MaaResource() = default;

    virtual bool set_option(MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size) = 0;

    virtual MaaResId post_path(const std::filesystem::path& path) = 0;

    virtual MaaStatus status(MaaResId res_id) const = 0;
    virtual MaaStatus wait(MaaResId res_id) const = 0;
    virtual MaaBool valid() const = 0;
    virtual MaaBool running() const = 0;
    virtual MaaBool clear() = 0;

    virtual void register_custom_recognition(const std::string& name, MaaCustomRecognitionCallback recognition, void* trans_arg) = 0;
    virtual void unregister_custom_recognition(const std::string& name) = 0;
    virtual void clear_custom_recognition() = 0;
    virtual void register_custom_action(const std::string& name, MaaCustomActionCallback action, void* trans_arg) = 0;
    virtual void unregister_custom_action(const std::string& name) = 0;
    virtual void clear_custom_action() = 0;

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
    virtual MaaCtrlId post_input_text(const std::string& text) = 0;
    virtual MaaCtrlId post_start_app(const std::string& intent) = 0;
    virtual MaaCtrlId post_stop_app(const std::string& intent) = 0;
    virtual MaaCtrlId post_screencap() = 0;

    virtual MaaCtrlId post_touch_down(int contact, int x, int y, int pressure) = 0;
    virtual MaaCtrlId post_touch_move(int contact, int x, int y, int pressure) = 0;
    virtual MaaCtrlId post_touch_up(int contact) = 0;

    virtual MaaStatus status(MaaCtrlId ctrl_id) const = 0;
    virtual MaaStatus wait(MaaCtrlId ctrl_id) const = 0;
    virtual MaaBool connected() const = 0;
    virtual MaaBool running() const = 0;

    virtual cv::Mat cached_image() const = 0;
    virtual std::string get_uuid() = 0;
};

struct MaaTasker
{
public:
    virtual ~MaaTasker() = default;

    virtual bool bind_resource(MaaResource* resource) = 0;
    virtual bool bind_controller(MaaController* controller) = 0;
    virtual bool inited() const = 0;

    virtual bool set_option(MaaTaskerOption key, MaaOptionValue value, MaaOptionValueSize val_size) = 0;

    virtual MaaTaskId post_pipeline(const std::string& entry, const json::value& pipeline_override) = 0;

    virtual MaaStatus status(MaaTaskId task_id) const = 0;
    virtual MaaStatus wait(MaaTaskId task_id) const = 0;

    virtual MaaBool running() const = 0;
    virtual void post_stop() = 0;

    virtual MaaResource* resource() const = 0;
    virtual MaaController* controller() const = 0;

    virtual void clear_cache() = 0;
    virtual std::optional<MAA_TASK_NS::TaskDetail> get_task_detail(MaaTaskId task_id) const = 0;
    virtual std::optional<MAA_TASK_NS::NodeDetail> get_node_detail(MaaNodeId node_id) const = 0;
    virtual std::optional<MAA_TASK_NS::RecoResult> get_reco_result(MaaRecoId reco_id) const = 0;
    virtual std::optional<MaaNodeId> get_latest_node(const std::string& task_name) const = 0;
};

struct MaaContext
{
public:
    virtual ~MaaContext() = default;

    virtual MaaTaskId run_pipeline(const std::string& entry, const json::value& pipeline_override) = 0;
    virtual MaaRecoId run_recognition(const std::string& entry, const json::value& pipeline_override, const cv::Mat& image) = 0;
    virtual MaaNodeId
        run_action(const std::string& entry, const json::value& pipeline_override, const cv::Rect& box, const std::string& reco_detail) = 0;
    virtual bool override_pipeline(const json::value& pipeline_override) = 0;
    virtual bool override_next(const std::string& name, const std::vector<std::string>& next) = 0;

    virtual MaaContext* clone() const = 0;

    virtual MaaTaskId task_id() const = 0;
    virtual MaaTasker* tasker() const = 0;
};
