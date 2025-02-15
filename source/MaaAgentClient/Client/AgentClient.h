#pragma once

#include <filesystem>

#include <meojson/json.hpp>
#include <zmq.hpp>

#include "API/MaaAgentClientTypes.h"
#include "Conf/Conf.h"
#include "Utils/IOStream/BoostIO.hpp"
#include "Utils/MessageNotifier.hpp"

MAA_AGENT_CLIENT_NS_BEGIN

class AgentClient : public MaaAgentClient
{
public:
    AgentClient(MaaNotificationCallback notify, void* notify_trans_arg);
    virtual ~AgentClient() override;

    virtual bool bind_resource(MaaResource* resource) override;
    virtual bool start_clild(const std::filesystem::path& child_exec, const std::vector<std::string>& child_args) override;

public:
    std::string create_socket();

    std::optional<json::value> recv();

    template <typename T>
    std::optional<T> recv()
    {
        auto jopt = recv();
        if (!jopt) {
            return std::nullopt;
        }
        const json::value& j = *jopt;
        return j.is<T>() ? std::make_optional(j.as<T>()) : std::nullopt;
    }

    bool recv_and_handle_init_msg();

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

private:
    MaaResource* resource_ = nullptr;
    MessageNotifier notifier_;
    boost::process::child child_;

    zmq::context_t child_ctx_;
    zmq::socket_t child_sock_;
};

MAA_AGENT_CLIENT_NS_END
