#pragma

#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <meojson/json.hpp>
#include <zmq.hpp>

#include "Conf/Conf.h"
#include "MaaAgentServer/MaaAgentServerDef.h"
#include "Utils/SingletonHolder.hpp"

MAA_AGENT_SERVER_NS_BEGIN

class AgentServer : public SingletonHolder<AgentServer>
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

    bool start_up(const std::vector<std::string>& args);
    void shut_down();
    void join();
    void detach();

    bool register_custom_recognition(const std::string& name, MaaCustomRecognitionCallback recognition, void* trans_arg);
    bool register_custom_action(const std::string& name, MaaCustomActionCallback action, void* trans_arg);

private:
    bool create_socket(const std::string& ipc_addr);

    bool send(const json::value& j);
    bool send_start_up_response();

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

    bool recv_and_handle_recognition_request(const json::value& j);
    bool recv_and_handle_action_request(const json::value& j);
    bool recv_and_handle_shut_down_request(const json::value& j);

    void request_msg_loop();

private:
    std::unordered_map<std::string, CustomRecognitionSession> custom_recognitions_;
    std::unordered_map<std::string, CustomActionSession> custom_actions_;

    std::string ipc_addr_;
    zmq::context_t parent_ctx_;
    zmq::socket_t parent_sock_;

    bool msg_loop_running_ = false;
    std::thread msg_thread_;
};

MAA_AGENT_SERVER_NS_END
