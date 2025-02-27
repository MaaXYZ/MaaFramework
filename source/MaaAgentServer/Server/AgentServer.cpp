#include "AgentServer.h"

#include <ranges>

#include "MaaAgent/Message.hpp"
#include "RemoteInstance/RemoteContext.h"
#include "Utils/Buffer/ImageBuffer.hpp"
#include "Utils/Buffer/StringBuffer.hpp"
#include "Utils/Codec.h"
#include "Utils/Logger.h"

MAA_AGENT_SERVER_NS_BEGIN

bool AgentServer::start_up(const std::vector<std::string>& args)
{
    LogFunc << VAR(args);

    if (args.empty()) {
        LogError << "args is empty";
        return false;
    }

    ipc_addr_ = args.back();
    LogInfo << VAR(ipc_addr_);

    if (ipc_addr_.empty()) {
        LogError << "ipc_addr is empty";
        return false;
    }

    bool socket_created = create_socket(ipc_addr_);
    if (!socket_created) {
        LogError << "failed to create_socket" << VAR(ipc_addr_);
        return false;
    }

    bool response_sent = send_start_up_response();
    if (!response_sent) {
        LogError << "failed to send_start_up_response";
        return false;
    }

    msg_loop_running_ = true;
    msg_thread_ = std::thread(&AgentServer::request_msg_loop, this);
    if (!msg_thread_.joinable()) {
        LogError << "failed to start msg_thread";
        return false;
    }

    return true;
}

void AgentServer::shut_down()
{
    LogFunc << VAR(ipc_addr_);

    msg_loop_running_ = false;

    if (msg_thread_.joinable()) {
        msg_thread_.join();
    }
}

void AgentServer::join()
{
    LogFunc << VAR(ipc_addr_);

    if (!msg_thread_.joinable()) {
        LogError << "msg_thread is not joinable";
        return;
    }

    msg_thread_.join();
}

void AgentServer::detach()
{
    LogFunc << VAR(ipc_addr_);

    if (!msg_thread_.joinable()) {
        LogError << "msg_thread is not joinable";
        return;
    }

    msg_thread_.detach();
}

bool AgentServer::register_custom_recognition(const std::string& name, MaaCustomRecognitionCallback recognition, void* trans_arg)
{
    LogInfo << VAR(name) << VAR_VOIDP(recognition) << VAR_VOIDP(trans_arg);

    if (name.empty() || recognition == nullptr) {
        LogError << "name or recognition is null";
        return false;
    }

    return custom_recognitions_.insert_or_assign(name, CustomRecognitionSession { recognition, trans_arg }).second;
}

bool AgentServer::register_custom_action(const std::string& name, MaaCustomActionCallback action, void* trans_arg)
{
    LogInfo << VAR(name) << VAR_VOIDP(action) << VAR_VOIDP(trans_arg);

    if (name.empty() || action == nullptr) {
        LogError << "name or action is null";
        return false;
    }

    return custom_actions_.insert_or_assign(name, CustomActionSession { action, trans_arg }).second;
}

bool AgentServer::create_socket(const std::string& ipc_addr)
{
    LogFunc << VAR(ipc_addr);

    parent_sock_ = zmq::socket_t(parent_ctx_, zmq::socket_type::pair);
    parent_sock_.connect(ipc_addr);
    return true;
}

bool AgentServer::send(const json::value& j)
{
    LogTrace << VAR(j) << VAR(ipc_addr_);

    std::string jstr = j.dumps();
    zmq::message_t msg(jstr.size());
    std::memcpy(msg.data(), jstr.data(), jstr.size());
    bool sent = parent_sock_.send(msg, zmq::send_flags::dontwait).has_value();
    if (!sent) {
        LogError << "failed to send msg" << VAR(j) << VAR(ipc_addr_);
        return false;
    }

    return true;
}

bool AgentServer::send_start_up_response()
{
    LogFunc << VAR(ipc_addr_);

    auto action_names = custom_actions_ | std::views::keys;
    auto reco_names = custom_recognitions_ | std::views::keys;

    StartUpResponse msg {
        .version = MAA_VERSION,
        .actions = { action_names.begin(), action_names.end() },
        .recognitions = { reco_names.begin(), reco_names.end() },
    };

    return send(msg);
}

std::optional<json::value> AgentServer::recv()
{
    LogFunc << VAR(ipc_addr_);

    zmq::message_t connected_msg;
    auto size_opt = parent_sock_.recv(connected_msg);
    if (!size_opt || *size_opt == 0) {
        LogError << "failed to recv connected_msg" << VAR(ipc_addr_);
        return std::nullopt;
    }

    std::string_view init_str = connected_msg.to_string_view();
    LogTrace << VAR(init_str);

    auto jopt = json::parse(init_str);
    if (!jopt) {
        LogError << "failed to parse connected_msg" << VAR(ipc_addr_);
        return std::nullopt;
    }

    return *jopt;
}

bool AgentServer::handle_inserted_request(const json::value& j)
{
    LogInfo << VAR(j) << VAR(ipc_addr_);

    if (handle_recognition_request(j)) {
        return true;
    }
    else if (handle_action_request(j)) {
        return true;
    }
    else if (handle_shut_down_request(j)) {
        return true;
    }
    else {
        LogError << "unexpected msg" << VAR(j);
        return false;
    }
}

bool AgentServer::handle_recognition_request(const json::value& j)
{
    if (!j.is<CustomRecognitionRequest>()) {
        return false;
    }

    const CustomRecognitionRequest& req = j.as<CustomRecognitionRequest>();
    LogInfo << VAR(req) << VAR(ipc_addr_);

    auto it = custom_recognitions_.find(req.custom_recognition_name);
    if (it == custom_recognitions_.end()) {
        LogError << "custom_recognition not found" << VAR(req);
        return true;
    }

    const CustomRecognitionSession& session = it->second;
    if (!session.recognition) {
        LogError << "recognition is null" << VAR(req);
        return true;
    }

    RemoteContext context(*this, req.context_id);
    cv::Mat mat = decode_image(req.image);
    ImageBuffer mat_buffer(mat);
    MaaRect rect { req.roi[0], req.roi[1], req.roi[2], req.roi[3] };

    MaaRect out_box {};
    StringBuffer out_detail;

    MaaBool ret = session.recognition(
        &context,
        req.task_id,
        req.node_name.c_str(),
        req.custom_recognition_name.c_str(),
        req.custom_recognition_param.c_str(),
        &mat_buffer,
        &rect,
        session.trans_arg,
        &out_box,
        &out_detail);

    CustomRecognitionResponse resp {
        .ret = static_cast<bool>(ret),
        .out_box = { out_box.x, out_box.y, out_box.width, out_box.height },
        .out_detail = out_detail.get(),
    };
    LogInfo << VAR(resp) << VAR(ipc_addr_);

    send(resp);

    return true;
}

bool AgentServer::handle_action_request(const json::value& j)
{
    if (!j.is<CustomActionRequest>()) {
        return false;
    }

    const CustomActionRequest& req = j.as<CustomActionRequest>();
    LogInfo << VAR(req) << VAR(ipc_addr_);

    auto it = custom_actions_.find(req.custom_action_name);
    if (it == custom_actions_.end()) {
        LogError << "custom_action not found" << VAR(req);
        return true;
    }

    const CustomActionSession& session = it->second;
    if (!session.action) {
        LogError << "action is null" << VAR(req);
        return true;
    }

    RemoteContext context(*this, req.context_id);
    MaaRect rect { req.box[0], req.box[1], req.box[2], req.box[3] };

    MaaBool ret = session.action(
        &context,
        req.task_id,
        req.node_name.c_str(),
        req.custom_action_name.c_str(),
        req.custom_action_param.c_str(),
        req.reco_id,
        &rect,
        session.trans_arg);

    CustomActionResponse resp {
        .ret = static_cast<bool>(ret),
    };
    LogInfo << VAR(resp) << VAR(ipc_addr_);

    send(resp);

    return true;
}

bool AgentServer::handle_shut_down_request(const json::value& j)
{
    if (!j.is<ShutDownRequest>()) {
        return false;
    }

    LogInfo << VAR(ipc_addr_);

    msg_loop_running_ = false;

    send(ShutDownResponse {});

    return true;
}

void AgentServer::request_msg_loop()
{
    LogFunc << VAR(ipc_addr_);

    while (msg_loop_running_) {
        auto msg_opt = recv();
        if (!msg_opt) {
            LogError << "failed to recv msg" << VAR(ipc_addr_);
            return;
        }
        const json::value& j = *msg_opt;
        handle_inserted_request(j);
    }
}

MAA_AGENT_SERVER_NS_END
