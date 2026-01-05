#include "AgentServer.h"

#include <ranges>

#include "MaaAgent/Message.hpp"
#include "MaaUtils/Buffer/ImageBuffer.hpp"
#include "MaaUtils/Buffer/StringBuffer.hpp"
#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "RemoteInstance/RemoteContext.h"

MAA_AGENT_SERVER_NS_BEGIN

bool AgentServer::start_up(const std::string& identifier)
{
    LogFunc << VAR(identifier);

    if (identifier.empty()) {
        LogError << "identifier is empty";
        return false;
    }

    init_socket(identifier, false);

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

    zmq_sock_.close();
    zmq_ctx_.close();
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

MaaSinkId AgentServer::add_resource_sink(MaaEventCallback sink, void* trans_arg)
{
    return res_notifier_.add_sink(sink, trans_arg);
}

MaaSinkId AgentServer::add_controller_sink(MaaEventCallback sink, void* trans_arg)
{
    return ctrl_notifier_.add_sink(sink, trans_arg);
}

MaaSinkId AgentServer::add_tasker_sink(MaaEventCallback sink, void* trans_arg)
{
    return tasker_notifier_.add_sink(sink, trans_arg);
}

MaaSinkId AgentServer::add_context_sink(MaaEventCallback sink, void* trans_arg)
{
    return ctx_notifier_.add_sink(sink, trans_arg);
}

bool AgentServer::handle_inserted_request(const json::value& j)
{
    LogInfo << VAR(j) << VAR(ipc_addr_);

    if (handle_image_header(j)) {
        return true;
    }
    else if (handle_image_encoded_header(j)) {
        return true;
    }
    else if (handle_recognition_request(j)) {
        return true;
    }
    else if (handle_action_request(j)) {
        return true;
    }
    else if (handle_resource_event(j)) {
        return true;
    }
    else if (handle_controller_event(j)) {
        return true;
    }
    else if (handle_tasker_event(j)) {
        return true;
    }
    else if (handle_context_event(j)) {
        return true;
    }
    else if (handle_start_up_request(j)) {
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
    cv::Mat mat = get_image_cache(req.image);
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

bool AgentServer::handle_start_up_request(const json::value& j)
{
    if (!j.is<StartUpRequest>()) {
        return false;
    }

    const StartUpRequest& req = j.as<StartUpRequest>();
    LogInfo << VAR(req) << VAR(ipc_addr_);

    if (req.protocol != kProtocolVersion) {
        LogError << "Protocol version mismatch" << "client:" << VAR(req.version) << VAR(req.protocol) << "server:" << VAR(MAA_VERSION)
                 << VAR(kProtocolVersion) << VAR(ipc_addr_);
        LogError << "Please update" << (req.protocol < kProtocolVersion ? "AgentClient" : "AgentServer");
    }

    auto action_names = custom_actions_ | std::views::keys;
    auto reco_names = custom_recognitions_ | std::views::keys;

    StartUpResponse msg {
        .actions = { action_names.begin(), action_names.end() },
        .recognitions = { reco_names.begin(), reco_names.end() },
    };

    return send(msg);
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

bool AgentServer::handle_resource_event(const json::value& j)
{
    if (!j.is<ResourceEventRequest>()) {
        return false;
    }

    const ResourceEventRequest& req = j.as<ResourceEventRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_) << VAR(req.message);

    RemoteResource resource(*this, req.resource_id);
    res_notifier_.notify(&resource, req.message, req.details);

    send(ResourceEventResponse {});

    return true;
}

bool AgentServer::handle_controller_event(const json::value& j)
{
    if (!j.is<ControllerEventRequest>()) {
        return false;
    }
    const ControllerEventRequest& req = j.as<ControllerEventRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_) << VAR(req.message);

    RemoteController controller(*this, req.controller_id);
    ctrl_notifier_.notify(&controller, req.message, req.details);

    send(ControllerEventResponse {});

    return true;
}

bool AgentServer::handle_tasker_event(const json::value& j)
{
    if (!j.is<TaskerEventRequest>()) {
        return false;
    }
    const TaskerEventRequest& req = j.as<TaskerEventRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_) << VAR(req.message);

    RemoteTasker tasker(*this, req.tasker_id);
    tasker_notifier_.notify(&tasker, req.message, req.details);

    send(TaskerEventResponse {});

    return true;
}

bool AgentServer::handle_context_event(const json::value& j)
{
    if (!j.is<ContextEventRequest>()) {
        return false;
    }
    const ContextEventRequest& req = j.as<ContextEventRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_) << VAR(req.message);

    RemoteContext context(*this, req.context_id);
    ctx_notifier_.notify(&context, req.message, req.details);

    send(ContextEventResponse {});

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
