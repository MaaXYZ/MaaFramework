#pragma once

#include <filesystem>

#include <meojson/json.hpp>
#include <zmq.hpp>

#include "API/MaaAgentClientTypes.h"
#include "Conf/Conf.h"
#include "Utils/IOStream/BoostIO.hpp"
#include "Utils/MessageNotifier.hpp"
#include "MaaAgent/Transceiver.h"

MAA_AGENT_CLIENT_NS_BEGIN

class AgentClient : public MaaAgentClient, public Transceiver
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

private:
    MaaResource* resource_ = nullptr;
    MessageNotifier notifier_;
    boost::process::child child_;
    std::string ipc_addr_;

    zmq::context_t child_ctx_;
    zmq::socket_t child_sock_;

    std::map<std::string, MaaContext*> context_map_;
};

MAA_AGENT_CLIENT_NS_END
