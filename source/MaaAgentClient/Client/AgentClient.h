#pragma once

#include <filesystem>

#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "Common/MaaTypes.h"
#include "MaaAgent/Transceiver.h"

MAA_AGENT_CLIENT_NS_BEGIN

class AgentClient
    : public MaaAgentClient
    , public Transceiver
{
public:
    AgentClient() = default;
    AgentClient(const std::string& identifier);
    virtual ~AgentClient() override;

public: // MaaAgentClient
    virtual std::string identifier() const override;
    virtual bool bind_resource(MaaResource* resource) override;
    virtual void register_resource_sink(MaaResource* res) override;
    virtual void register_controller_sink(MaaController* ctrl) override;
    virtual void register_tasker_sink(MaaTasker* tasker) override;
    virtual std::string create_socket(const std::string& identifier) override;
    virtual bool connect() override;
    virtual bool disconnect() override;
    virtual bool connected() override;
    virtual bool alive() override;
    virtual void set_timeout(const std::chrono::milliseconds& timeout) override;
    virtual std::vector<std::string> get_custom_recognition_list() const override;
    virtual std::vector<std::string> get_custom_action_list() const override;

private: // Transceiver
    virtual bool handle_inserted_request(const json::value& j) override;

private:
    bool handle_context_run_task(const json::value& j);
    bool handle_context_run_recognition(const json::value& j);
    bool handle_context_run_action(const json::value& j);
    bool handle_context_override_pipeline(const json::value& j);
    bool handle_context_override_next(const json::value& j);
    bool handle_context_override_image(const json::value& j);
    bool handle_context_get_node_data(const json::value& j);
    bool handle_context_clone(const json::value& j);
    bool handle_context_task_id(const json::value& j);
    bool handle_context_tasker(const json::value& j);
    bool handle_context_set_anchor(const json::value& j);
    bool handle_context_get_anchor(const json::value& j);
    bool handle_context_get_hit_count(const json::value& j);
    bool handle_context_clear_hit_count(const json::value& j);

    bool handle_tasker_inited(const json::value& j);
    bool handle_tasker_post_task(const json::value& j);
    bool handle_tasker_post_recognition(const json::value& j);
    bool handle_tasker_post_action(const json::value& j);
    bool handle_tasker_status(const json::value& j);
    bool handle_tasker_wait(const json::value& j);
    bool handle_tasker_running(const json::value& j);
    bool handle_tasker_post_stop(const json::value& j);
    bool handle_tasker_stopping(const json::value& j);
    bool handle_tasker_resource(const json::value& j);
    bool handle_tasker_controller(const json::value& j);
    bool handle_tasker_clear_cache(const json::value& j);
    bool handle_tasker_get_task_detail(const json::value& j);
    bool handle_tasker_get_node_detail(const json::value& j);
    bool handle_tasker_get_reco_result(const json::value& j);
    bool handle_tasker_get_action_result(const json::value& j);
    bool handle_tasker_get_latest_node(const json::value& j);

    bool handle_resource_post_bundle(const json::value& j);
    bool handle_resource_post_ocr_model(const json::value& j);
    bool handle_resource_post_pipeline(const json::value& j);
    bool handle_resource_post_image(const json::value& j);
    bool handle_resource_status(const json::value& j);
    bool handle_resource_wait(const json::value& j);
    bool handle_resource_valid(const json::value& j);
    bool handle_resource_running(const json::value& j);
    bool handle_resource_clear(const json::value& j);
    bool handle_resource_override_pipeline(const json::value& j);
    bool handle_resource_override_next(const json::value& j);
    bool handle_resource_override_image(const json::value& j);
    bool handle_resource_get_node_data(const json::value& j);
    bool handle_resource_get_hash(const json::value& j);
    bool handle_resource_get_node_list(const json::value& j);
    bool handle_resource_get_custom_recognition_list(const json::value& j);
    bool handle_resource_get_custom_action_list(const json::value& j);

    bool handle_controller_post_connection(const json::value& j);
    bool handle_controller_post_click(const json::value& j);
    bool handle_controller_post_swipe(const json::value& j);
    bool handle_controller_post_click_key(const json::value& j);
    bool handle_controller_post_input_text(const json::value& j);
    bool handle_controller_post_start_app(const json::value& j);
    bool handle_controller_post_stop_app(const json::value& j);
    bool handle_controller_post_screencap(const json::value& j);
    bool handle_controller_post_shell(const json::value& j);
    bool handle_controller_post_touch_down(const json::value& j);
    bool handle_controller_post_touch_move(const json::value& j);
    bool handle_controller_post_touch_up(const json::value& j);
    bool handle_controller_post_key_down(const json::value& j);
    bool handle_controller_post_key_up(const json::value& j);
    bool handle_controller_post_scroll(const json::value& j);
    bool handle_controller_status(const json::value& j);
    bool handle_controller_wait(const json::value& j);
    bool handle_controller_connected(const json::value& j);
    bool handle_controller_running(const json::value& j);
    bool handle_controller_cached_image(const json::value& j);
    bool handle_controller_get_shell_output(const json::value& j);
    bool handle_controller_get_uuid(const json::value& j);

    bool handle_event_response(const json::value& j);

public:
    static MaaBool reco_agent(
        MaaContext* context,
        MaaTaskId task_id,
        const char* node_name,
        const char* custom_recognition_name,
        const char* custom_recognition_param,
        const MaaImageBuffer* image,
        const MaaRect* roi,
        void* trans_arg,
        /* out */ MaaRect* out_box,
        /* out */ MaaStringBuffer* out_detail);

    static MaaBool action_agent(
        MaaContext* context,
        MaaTaskId task_id,
        const char* node_name,
        const char* custom_action_name,
        const char* custom_action_param,
        MaaRecoId reco_id,
        const MaaRect* box,
        void* trans_arg);

    std::string context_id(MaaContext* context);
    MaaContext* query_context(const std::string& context_id);

    std::string tasker_id(MaaTasker* tasker);
    MaaTasker* query_tasker(const std::string& tasker_id);

    std::string controller_id(MaaController* controller);
    MaaController* query_controller(const std::string& controller_id);

    std::string resource_id(MaaResource* resource);
    MaaResource* query_resource(const std::string& resource_id);

    void clear_custom_registration();
    void clear_resource_sink();
    void clear_controller_sink();
    void clear_tasker_sink();

public:
    static void res_event_sink(void* handle, const char* message, const char* details_json, void* trans_arg);
    static void ctrl_event_sink(void* handle, const char* message, const char* details_json, void* trans_arg);
    static void tasker_event_sink(void* handle, const char* message, const char* details_json, void* trans_arg);
    static void ctx_event_sink(void* handle, const char* message, const char* details_json, void* trans_arg);

private:
    // for bind_resource
    MaaResource* bound_res_ = nullptr;

    // for register_*_sink functions
    MaaTasker* reg_tasker_ = nullptr;
    MaaResource* reg_res_ = nullptr;
    MaaController* reg_ctrl_ = nullptr;
    MaaSinkId reg_tasker_sink_id_ = MaaInvalidId;
    MaaSinkId reg_context_sink_id_ = MaaInvalidId;
    MaaSinkId reg_res_sink_id_ = MaaInvalidId;
    MaaSinkId reg_ctrl_sink_id_ = MaaInvalidId;

    bool connected_ = false;
    std::string identifier_;

    std::map<std::string, MaaContext*> context_map_;
    std::map<std::string, MaaTasker*> tasker_map_;
    std::map<std::string, MaaController*> controller_map_;
    std::map<std::string, MaaResource*> resource_map_;

    std::vector<std::string> registered_actions_;
    std::vector<std::string> registered_recognitions_;
};

MAA_AGENT_CLIENT_NS_END
