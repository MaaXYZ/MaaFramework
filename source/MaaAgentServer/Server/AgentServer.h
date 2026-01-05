#pragma once

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <meojson/json.hpp>
#include <zmq.hpp>

#include "Common/Conf.h"
#include "MaaAgent/Transceiver.h"
#include "MaaAgentServer/MaaAgentServerDef.h"
#include "MaaUtils/SingletonHolder.hpp"
#include "Utils/EventDispatcher.hpp"

MAA_AGENT_SERVER_NS_BEGIN

class AgentServer
    : public SingletonHolder<AgentServer>
    , public Transceiver
{
    struct CustomRecognitionSession
    {
        MaaCustomRecognitionCallback recognition = nullptr;
        void* trans_arg = nullptr;
    };

    struct CustomActionSession
    {
        MaaCustomActionCallback action = nullptr;
        void* trans_arg = nullptr;
    };

public:
    ~AgentServer() = default;

    bool start_up(const std::string& identifier);
    void shut_down();
    void join();
    void detach();

    bool register_custom_recognition(const std::string& name, MaaCustomRecognitionCallback recognition, void* trans_arg);
    bool register_custom_action(const std::string& name, MaaCustomActionCallback action, void* trans_arg);

    MaaSinkId add_resource_sink(MaaEventCallback sink, void* trans_arg);
    MaaSinkId add_controller_sink(MaaEventCallback sink, void* trans_arg);
    MaaSinkId add_tasker_sink(MaaEventCallback sink, void* trans_arg);
    MaaSinkId add_context_sink(MaaEventCallback sink, void* trans_arg);

public:
    virtual bool handle_inserted_request(const json::value& j) override;

private:
    bool handle_recognition_request(const json::value& j);
    bool handle_action_request(const json::value& j);
    bool handle_start_up_request(const json::value& j);
    bool handle_shut_down_request(const json::value& j);

    bool handle_resource_event(const json::value& j);
    bool handle_controller_event(const json::value& j);
    bool handle_tasker_event(const json::value& j);
    bool handle_context_event(const json::value& j);

    void request_msg_loop();

private:
    std::unordered_map<std::string, CustomRecognitionSession> custom_recognitions_;
    std::unordered_map<std::string, CustomActionSession> custom_actions_;

    EventDispatcher res_notifier_;
    EventDispatcher ctrl_notifier_;
    EventDispatcher tasker_notifier_;
    EventDispatcher ctx_notifier_;

    bool msg_loop_running_ = false;
    std::thread msg_thread_;
};

MAA_AGENT_SERVER_NS_END
