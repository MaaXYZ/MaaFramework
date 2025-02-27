#pragma once

#include <filesystem>

#include <meojson/json.hpp>
#include <zmq.hpp>

#include "API/MaaAgentClientTypes.h"
#include "Conf/Conf.h"
#include "MaaAgent/Transceiver.h"
#include "Utils/IOStream/BoostIO.hpp"
#include "Utils/MessageNotifier.hpp"

MAA_AGENT_CLIENT_NS_BEGIN

class AgentClient
    : public MaaAgentClient
    , public Transceiver
{
public:
    AgentClient(MaaNotificationCallback notify, void* notify_trans_arg);
    virtual ~AgentClient() override;

public: // MaaAgentClient
    virtual bool bind_resource(MaaResource* resource) override;
    virtual bool start_clild(const std::filesystem::path& child_exec, const std::vector<std::string>& child_args) override;

private: // Transceiver
    virtual bool send(const json::value& j) override;
    virtual std::optional<json::value> recv() override;
    virtual bool handle_inserted_request(const json::value& j) override;

private:
    std::string create_socket();

    bool recv_and_handle_start_up_response();

    bool handle_context_run_task(const json::value& j);
    bool handle_context_run_recognition(const json::value& j);
    bool handle_context_run_action(const json::value& j);
    bool handle_context_override_pipeline(const json::value& j);
    bool handle_context_override_next(const json::value& j);
    bool handle_context_clone(const json::value& j);
    bool handle_context_task_id(const json::value& j);
    bool handle_context_tasker(const json::value& j);

    bool handle_tasker_inited(const json::value& j);
    bool handle_tasker_post_task(const json::value& j);
    bool handle_tasker_status(const json::value& j);
    bool handle_tasker_wait(const json::value& j);
    bool handle_tasker_running(const json::value& j);
    bool handle_tasker_post_stop(const json::value& j);
    bool handle_tasker_resource(const json::value& j);
    bool handle_tasker_controller(const json::value& j);
    bool handle_tasker_clear_cache(const json::value& j);
    bool handle_tasker_get_task_detail(const json::value& j);
    bool handle_tasker_get_node_detail(const json::value& j);
    bool handle_tasker_get_reco_result(const json::value& j);
    bool handle_tasker_get_latest_node(const json::value& j);

    bool handle_resource_post_bundle(const json::value& j);
    bool handle_resource_status(const json::value& j);
    bool handle_resource_wait(const json::value& j);
    bool handle_resource_valid(const json::value& j);
    bool handle_resource_running(const json::value& j);
    bool handle_resource_clear(const json::value& j);
    bool handle_resource_get_hash(const json::value& j);
    bool handle_resource_get_node_list(const json::value& j);

    bool handle_controller_post_connection(const json::value& j);
    bool handle_controller_post_click(const json::value& j);
    bool handle_controller_post_swipe(const json::value& j);
    bool handle_controller_post_press_key(const json::value& j);
    bool handle_controller_post_input_text(const json::value& j);
    bool handle_controller_post_start_app(const json::value& j);
    bool handle_controller_post_stop_app(const json::value& j);
    bool handle_controller_post_screencap(const json::value& j);
    bool handle_controller_post_touch_down(const json::value& j);
    bool handle_controller_post_touch_move(const json::value& j);
    bool handle_controller_post_touch_up(const json::value& j);
    bool handle_controller_status(const json::value& j);
    bool handle_controller_wait(const json::value& j);
    bool handle_controller_connected(const json::value& j);
    bool handle_controller_running(const json::value& j);
    bool handle_controller_cached_image(const json::value& j);
    bool handle_controller_get_uuid(const json::value& j);

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

private:
    MaaResource* resource_ = nullptr;
    MessageNotifier notifier_;
    boost::process::child child_;
    std::string ipc_addr_;

    zmq::context_t child_ctx_;
    zmq::socket_t child_sock_;

    std::map<std::string, MaaContext*> context_map_;
    std::map<std::string, MaaTasker*> tasker_map_;
    std::map<std::string, MaaController*> controller_map_;
    std::map<std::string, MaaResource*> resource_map_;
};

MAA_AGENT_CLIENT_NS_END
