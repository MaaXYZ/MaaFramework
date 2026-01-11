#include "AgentClient.h"

#include <meojson/json.hpp>

#include "Common/MaaTypes.h"
#include "MaaAgent/Message.hpp"
#include "MaaFramework/MaaAPI.h"
#include "MaaUtils/Buffer/ImageBuffer.hpp"
#include "MaaUtils/Buffer/ListBuffer.hpp"
#include "MaaUtils/Buffer/StringBuffer.hpp"
#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/Uuid.h"

MAA_AGENT_CLIENT_NS_BEGIN

AgentClient::AgentClient(const std::string& identifier)
{
    LogFunc;

    identifier_ = identifier.empty() ? make_uuid() : identifier;
    init_socket(identifier_, true);

    LogInfo << VAR(identifier) << VAR(identifier_);
}

AgentClient::~AgentClient()
{
    LogFunc << VAR(ipc_addr_);

    clear_custom_registration();
    clear_controller_sink();
    clear_resource_sink();
    clear_tasker_sink();
}

std::string AgentClient::identifier() const
{
    return identifier_;
}

bool AgentClient::bind_resource(MaaResource* new_res)
{
    LogInfo << VAR_VOIDP(this) << VAR_VOIDP(new_res);

    if (bound_res_ && bound_res_ != new_res) {
        LogWarn << "resource is already bound" << VAR_VOIDP(bound_res_);
        clear_custom_registration();
    }

    bound_res_ = new_res;

    return true;
}

void AgentClient::register_resource_sink(MaaResource* res)
{
    LogInfo << VAR_VOIDP(this) << VAR_VOIDP(res);

    if (!res) {
        LogError << "resource is null";
        return;
    }

    clear_resource_sink();
    reg_res_sink_id_ = res->add_sink(&AgentClient::res_event_sink, this);
    reg_res_ = res;
}

void AgentClient::register_controller_sink(MaaController* ctrl)
{
    LogInfo << VAR_VOIDP(this) << VAR_VOIDP(ctrl);

    if (!ctrl) {
        LogError << "controller is null";
        return;
    }

    clear_controller_sink();
    reg_ctrl_sink_id_ = ctrl->add_sink(&AgentClient::ctrl_event_sink, this);
    reg_ctrl_ = ctrl;
}

void AgentClient::register_tasker_sink(MaaTasker* tasker)
{
    LogInfo << VAR_VOIDP(this) << VAR_VOIDP(tasker);

    if (!tasker) {
        LogError << "tasker is null";
        return;
    }

    clear_tasker_sink();
    reg_tasker_sink_id_ = tasker->add_sink(&AgentClient::tasker_event_sink, this);
    reg_context_sink_id_ = tasker->add_context_sink(&AgentClient::ctx_event_sink, this);
    reg_tasker_ = tasker;
}

std::string AgentClient::create_socket(const std::string& identifier)
{
    // Check if a socket has already been created.
    // If a socket exists, log an error and return the existing identifier.
    // Multiple socket creations are not allowed by design.
    if (!identifier_.empty()) {
        LogError << "Attempted to create a new socket, but one already exists. Returning the existing socket identifier."
                 << VAR(identifier_);
        return identifier_;
    }

    // Create a new socket with the provided identifier or generate a new one if empty.
    identifier_ = identifier.empty() ? make_uuid() : identifier;

    init_socket(identifier_, true);

    return identifier_;
}

bool AgentClient::connect()
{
    LogFunc << VAR(ipc_addr_);

    if (!bound_res_) {
        LogError << "resource is not bound";
        return false;
    }

    clear_custom_registration();

    auto resp_opt = send_and_recv<StartUpResponse>(StartUpRequest {});

    if (!resp_opt) {
        LogError << "failed to send_and_recv";
        return false;
    }
    const auto& resp = *resp_opt;
    LogInfo << VAR(resp);

    if (resp.protocol != kProtocolVersion) {
        LogError << "Protocol version mismatch" << "client:" << VAR(MAA_VERSION) << VAR(kProtocolVersion) << "server:" << VAR(resp.version)
                 << VAR(resp.protocol) << VAR(ipc_addr_);
        LogError << "Please update" << (kProtocolVersion < resp.protocol ? "AgentClient" : "AgentServer");
        return false;
    }

    for (const auto& reco : resp.recognitions) {
        LogInfo << "register recognition" << VAR(reco);
        bound_res_->register_custom_recognition(reco, reco_agent, this);
    }
    for (const auto& act : resp.actions) {
        LogInfo << "register action" << VAR(act);
        bound_res_->register_custom_action(act, action_agent, this);
    }

    registered_recognitions_ = resp.recognitions;
    registered_actions_ = resp.actions;

    connected_ = true;
    return true;
}

bool AgentClient::disconnect()
{
    LogFunc << VAR(ipc_addr_);

    clear_custom_registration();

    if (!connected()) {
        return true;
    }

    if (alive()) {
        send_and_recv<ShutDownResponse>(ShutDownRequest {});
    }

    connected_ = false;
    return true;
}

bool AgentClient::connected()
{
    return connected_;
}

bool AgentClient::alive()
{
    return Transceiver::alive();
}

void AgentClient::set_timeout(const std::chrono::milliseconds& timeout)
{
    Transceiver::set_timeout(timeout);
}

std::vector<std::string> AgentClient::get_custom_recognition_list() const
{
    return registered_recognitions_;
}

std::vector<std::string> AgentClient::get_custom_action_list() const
{
    return registered_actions_;
}

bool AgentClient::handle_inserted_request(const json::value& j)
{
    // LogFunc << VAR(j) << VAR(ipc_addr_);

    if (handle_image_header(j)) {
        return true;
    }
    else if (handle_image_encoded_header(j)) {
        return true;
    }

    else if (handle_context_run_task(j)) {
        return true;
    }
    else if (handle_context_run_recognition(j)) {
        return true;
    }
    else if (handle_context_run_action(j)) {
        return true;
    }
    else if (handle_context_override_pipeline(j)) {
        return true;
    }
    else if (handle_context_override_next(j)) {
        return true;
    }
    else if (handle_context_override_image(j)) {
        return true;
    }
    else if (handle_context_get_node_data(j)) {
        return true;
    }
    else if (handle_context_clone(j)) {
        return true;
    }
    else if (handle_context_task_id(j)) {
        return true;
    }
    else if (handle_context_tasker(j)) {
        return true;
    }
    else if (handle_context_set_anchor(j)) {
        return true;
    }
    else if (handle_context_get_anchor(j)) {
        return true;
    }
    else if (handle_context_get_hit_count(j)) {
        return true;
    }
    else if (handle_context_clear_hit_count(j)) {
        return true;
    }

    else if (handle_tasker_inited(j)) {
        return true;
    }
    else if (handle_tasker_post_task(j)) {
        return true;
    }
    else if (handle_tasker_post_recognition(j)) {
        return true;
    }
    else if (handle_tasker_post_action(j)) {
        return true;
    }
    else if (handle_tasker_status(j)) {
        return true;
    }
    else if (handle_tasker_wait(j)) {
        return true;
    }
    else if (handle_tasker_running(j)) {
        return true;
    }
    else if (handle_tasker_post_stop(j)) {
        return true;
    }
    else if (handle_tasker_stopping(j)) {
        return true;
    }
    else if (handle_tasker_resource(j)) {
        return true;
    }
    else if (handle_tasker_controller(j)) {
        return true;
    }
    else if (handle_tasker_clear_cache(j)) {
        return true;
    }
    else if (handle_tasker_get_task_detail(j)) {
        return true;
    }
    else if (handle_tasker_get_node_detail(j)) {
        return true;
    }
    else if (handle_tasker_get_reco_result(j)) {
        return true;
    }
    else if (handle_tasker_get_action_result(j)) {
        return true;
    }
    else if (handle_tasker_get_latest_node(j)) {
        return true;
    }

    else if (handle_resource_post_bundle(j)) {
        return true;
    }
    else if (handle_resource_post_ocr_model(j)) {
        return true;
    }
    else if (handle_resource_post_pipeline(j)) {
        return true;
    }
    else if (handle_resource_post_image(j)) {
        return true;
    }
    else if (handle_resource_status(j)) {
        return true;
    }
    else if (handle_resource_wait(j)) {
        return true;
    }
    else if (handle_resource_valid(j)) {
        return true;
    }
    else if (handle_resource_running(j)) {
        return true;
    }
    else if (handle_resource_clear(j)) {
        return true;
    }
    else if (handle_resource_override_pipeline(j)) {
        return true;
    }
    else if (handle_resource_override_next(j)) {
        return true;
    }
    else if (handle_resource_override_image(j)) {
        return true;
    }
    else if (handle_resource_get_node_data(j)) {
        return true;
    }
    else if (handle_resource_get_hash(j)) {
        return true;
    }
    else if (handle_resource_get_node_list(j)) {
        return true;
    }
    else if (handle_resource_get_custom_recognition_list(j)) {
        return true;
    }
    else if (handle_resource_get_custom_action_list(j)) {
        return true;
    }

    else if (handle_controller_post_connection(j)) {
        return true;
    }
    else if (handle_controller_post_click(j)) {
        return true;
    }
    else if (handle_controller_post_swipe(j)) {
        return true;
    }
    else if (handle_controller_post_click_key(j)) {
        return true;
    }
    else if (handle_controller_post_input_text(j)) {
        return true;
    }
    else if (handle_controller_post_start_app(j)) {
        return true;
    }
    else if (handle_controller_post_stop_app(j)) {
        return true;
    }
    else if (handle_controller_post_screencap(j)) {
        return true;
    }
    else if (handle_controller_post_shell(j)) {
        return true;
    }
    else if (handle_controller_post_touch_down(j)) {
        return true;
    }
    else if (handle_controller_post_touch_move(j)) {
        return true;
    }
    else if (handle_controller_post_touch_up(j)) {
        return true;
    }
    else if (handle_controller_post_key_down(j)) {
        return true;
    }
    else if (handle_controller_post_key_up(j)) {
        return true;
    }
    else if (handle_controller_post_scroll(j)) {
        return true;
    }
    else if (handle_controller_status(j)) {
        return true;
    }
    else if (handle_controller_wait(j)) {
        return true;
    }
    else if (handle_controller_connected(j)) {
        return true;
    }
    else if (handle_controller_running(j)) {
        return true;
    }
    else if (handle_controller_cached_image(j)) {
        return true;
    }
    else if (handle_controller_get_shell_output(j)) {
        return true;
    }
    else if (handle_controller_get_uuid(j)) {
        return true;
    }
    else if (handle_event_response(j)) {
        return true;
    }
    else {
        LogError << "unexpected msg" << VAR(j) << VAR(ipc_addr_);
        return false;
    }
}

bool AgentClient::handle_context_run_task(const json::value& j)
{
    if (!j.is<ContextRunTaskReverseRequest>()) {
        return false;
    }

    const ContextRunTaskReverseRequest& req = j.as<ContextRunTaskReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaContext* context = query_context(req.context_id);
    if (!context) {
        LogError << "context not found" << VAR(req.context_id);
        return false;
    }

    auto task_id = context->run_task(req.entry, req.pipeline_override);

    ContextRunTaskReverseResponse resp {
        .task_id = task_id,
    };
    send(resp);

    return true;
}

bool AgentClient::handle_context_run_recognition(const json::value& j)
{
    if (!j.is<ContextRunRecognitionReverseRequest>()) {
        return false;
    }

    const ContextRunRecognitionReverseRequest& req = j.as<ContextRunRecognitionReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaContext* context = query_context(req.context_id);
    if (!context) {
        LogError << "context not found" << VAR(req.context_id);
        return false;
    }

    MaaRecoId reco_id = context->run_recognition(req.entry, req.pipeline_override, get_image_cache(req.image));

    ContextRunRecognitionReverseResponse resp {
        .reco_id = reco_id,
    };
    send(resp);

    return true;
}

bool AgentClient::handle_context_run_action(const json::value& j)
{
    if (!j.is<ContextRunActionReverseRequest>()) {
        return false;
    }

    const ContextRunActionReverseRequest& req = j.as<ContextRunActionReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaContext* context = query_context(req.context_id);
    if (!context) {
        LogError << "context not found" << VAR(req.context_id);
        return false;
    }

    MaaActId act_id =
        context->run_action(req.entry, req.pipeline_override, cv::Rect { req.box[0], req.box[1], req.box[2], req.box[3] }, req.reco_detail);

    ContextRunActionReverseResponse resp {
        .action_id = act_id,
    };
    send(resp);

    return true;
}

bool AgentClient::handle_context_override_pipeline(const json::value& j)
{
    if (!j.is<ContextOverridePipelineReverseRequest>()) {
        return false;
    }

    const ContextOverridePipelineReverseRequest& req = j.as<ContextOverridePipelineReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaContext* context = query_context(req.context_id);
    if (!context) {
        LogError << "context not found" << VAR(req.context_id);
        return false;
    }

    bool ret = context->override_pipeline(req.pipeline_override);

    ContextOverridePipelineReverseResponse resp {
        .ret = ret,
    };
    send(resp);

    return true;
}

bool AgentClient::handle_context_override_next(const json::value& j)
{
    if (!j.is<ContextOverrideNextReverseRequest>()) {
        return false;
    }

    const ContextOverrideNextReverseRequest& req = j.as<ContextOverrideNextReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaContext* context = query_context(req.context_id);
    if (!context) {
        LogError << "context not found" << VAR(req.context_id);
        return false;
    }

    bool ret = context->override_next(req.node_name, req.next);

    ContextOverrideNextReverseResponse resp {
        .ret = ret,
    };
    send(resp);

    return true;
}

bool AgentClient::handle_context_override_image(const json::value& j)
{
    if (!j.is<ContextOverrideImageReverseRequest>()) {
        return false;
    }

    const ContextOverrideImageReverseRequest& req = j.as<ContextOverrideImageReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaContext* context = query_context(req.context_id);
    if (!context) {
        LogError << "context not found" << VAR(req.context_id);
        return false;
    }

    cv::Mat image = get_image_cache(req.image);
    bool ret = context->override_image(req.image_name, image);

    ContextOverrideImageReverseResponse resp {
        .ret = ret,
    };
    send(resp);

    return true;
}

bool AgentClient::handle_context_get_node_data(const json::value& j)
{
    if (!j.is<ContextGetNodeDataReverseRequest>()) {
        return false;
    }

    const ContextGetNodeDataReverseRequest& req = j.as<ContextGetNodeDataReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaContext* context = query_context(req.context_id);
    if (!context) {
        LogError << "context not found" << VAR(req.context_id);
        return false;
    }

    auto opt = context->get_node_data(req.node_name);

    ContextGetNodeDataReverseResponse resp {
        .has_value = opt.has_value(),
        .node_data = opt ? *opt : json::object(),
    };
    send(resp);

    return true;
}

bool AgentClient::handle_context_clone(const json::value& j)
{
    if (!j.is<ContextCloneReverseRequest>()) {
        return false;
    }

    const ContextCloneReverseRequest& req = j.as<ContextCloneReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaContext* context = query_context(req.context_id);
    if (!context) {
        LogError << "context not found" << VAR(req.context_id);
        return false;
    }

    MaaContext* clone = context->clone();

    ContextCloneReverseResponse resp {
        .clone_id = context_id(clone),
    };
    send(resp);

    return true;
}

bool AgentClient::handle_context_task_id(const json::value& j)
{
    if (!j.is<ContextTaskIdReverseRequest>()) {
        return false;
    }

    const ContextTaskIdReverseRequest& req = j.as<ContextTaskIdReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaContext* context = query_context(req.context_id);
    if (!context) {
        LogError << "context not found" << VAR(req.context_id);
        return false;
    }

    MaaTaskId task_id = context->task_id();

    ContextTaskIdReverseResponse resp {
        .task_id = task_id,
    };
    send(resp);

    return true;
}

bool AgentClient::handle_context_tasker(const json::value& j)
{
    if (!j.is<ContextTaskerReverseRequest>()) {
        return false;
    }

    const ContextTaskerReverseRequest& req = j.as<ContextTaskerReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaContext* context = query_context(req.context_id);
    if (!context) {
        LogError << "context not found" << VAR(req.context_id);
        return false;
    }

    MaaTasker* tasker = context->tasker();

    ContextTaskerReverseResponse resp {
        .tasker_id = tasker_id(tasker),
    };
    send(resp);
    return true;
}

bool AgentClient::handle_context_set_anchor(const json::value& j)
{
    if (!j.is<ContextSetAnchorReverseRequest>()) {
        return false;
    }

    const ContextSetAnchorReverseRequest& req = j.as<ContextSetAnchorReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaContext* context = query_context(req.context_id);
    if (!context) {
        LogError << "context not found" << VAR(req.context_id);
        return false;
    }

    context->set_anchor(req.anchor_name, req.node_name);

    ContextSetAnchorReverseResponse resp {};
    send(resp);
    return true;
}

bool AgentClient::handle_context_get_anchor(const json::value& j)
{
    if (!j.is<ContextGetAnchorReverseRequest>()) {
        return false;
    }

    const ContextGetAnchorReverseRequest& req = j.as<ContextGetAnchorReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaContext* context = query_context(req.context_id);
    if (!context) {
        LogError << "context not found" << VAR(req.context_id);
        return false;
    }

    auto opt = context->get_anchor(req.anchor_name);

    ContextGetAnchorReverseResponse resp {
        .has_value = opt.has_value(),
        .node_name = opt.value_or(""),
    };
    send(resp);
    return true;
}

bool AgentClient::handle_context_get_hit_count(const json::value& j)
{
    if (!j.is<ContextGetHitCountReverseRequest>()) {
        return false;
    }

    const ContextGetHitCountReverseRequest& req = j.as<ContextGetHitCountReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaContext* context = query_context(req.context_id);
    if (!context) {
        LogError << "context not found" << VAR(req.context_id);
        return false;
    }

    size_t count = context->get_hit_count(req.node_name);

    ContextGetHitCountReverseResponse resp {
        .count = count,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_context_clear_hit_count(const json::value& j)
{
    if (!j.is<ContextClearHitCountReverseRequest>()) {
        return false;
    }

    const ContextClearHitCountReverseRequest& req = j.as<ContextClearHitCountReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaContext* context = query_context(req.context_id);
    if (!context) {
        LogError << "context not found" << VAR(req.context_id);
        return false;
    }

    context->clear_hit_count(req.node_name);

    ContextClearHitCountReverseResponse resp {};
    send(resp);
    return true;
}

bool AgentClient::handle_tasker_inited(const json::value& j)
{
    if (!j.is<TaskerInitedReverseRequest>()) {
        return false;
    }
    const TaskerInitedReverseRequest& req = j.as<TaskerInitedReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }

    bool ret = tasker->inited();

    TaskerInitedReverseResponse resp {
        .ret = ret,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_tasker_post_task(const json::value& j)
{
    if (!j.is<TaskerPostTaskReverseRequest>()) {
        return false;
    }
    const TaskerPostTaskReverseRequest& req = j.as<TaskerPostTaskReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }

    MaaTaskId task_id = tasker->post_task(req.entry, req.pipeline_override);

    TaskerPostTaskReverseResponse resp {
        .task_id = task_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_tasker_post_recognition(const json::value& j)
{
    if (!j.is<TaskerPostRecognitionReverseRequest>()) {
        return false;
    }
    const TaskerPostRecognitionReverseRequest& req = j.as<TaskerPostRecognitionReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }

    MaaTaskId task_id = tasker->post_recognition(req.reco_type, req.reco_param, get_image_cache(req.image));

    TaskerPostRecognitionReverseResponse resp {
        .task_id = task_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_tasker_post_action(const json::value& j)
{
    if (!j.is<TaskerPostActionReverseRequest>()) {
        return false;
    }
    const TaskerPostActionReverseRequest& req = j.as<TaskerPostActionReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }

    MaaTaskId task_id = tasker->post_action(
        req.action_type,
        req.action_param,
        cv::Rect { req.box[0], req.box[1], req.box[2], req.box[3] },
        req.reco_detail);

    TaskerPostActionReverseResponse resp {
        .task_id = task_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_tasker_status(const json::value& j)
{
    if (!j.is<TaskerStatusReverseRequest>()) {
        return false;
    }
    const TaskerStatusReverseRequest& req = j.as<TaskerStatusReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }
    MaaStatus status = tasker->status(req.task_id);
    TaskerStatusReverseResponse resp {
        .status = status,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_tasker_wait(const json::value& j)
{
    if (!j.is<TaskerWaitReverseRequest>()) {
        return false;
    }
    const TaskerWaitReverseRequest& req = j.as<TaskerWaitReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }
    MaaStatus status = tasker->wait(req.task_id);
    TaskerWaitReverseResponse resp {
        .status = status,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_tasker_running(const json::value& j)
{
    if (!j.is<TaskerRunningReverseRequest>()) {
        return false;
    }
    const TaskerRunningReverseRequest& req = j.as<TaskerRunningReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }
    bool running = tasker->running();
    TaskerRunningReverseResponse resp {
        .ret = running,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_tasker_post_stop(const json::value& j)
{
    if (!j.is<TaskerPostStopReverseRequest>()) {
        return false;
    }
    const TaskerPostStopReverseRequest& req = j.as<TaskerPostStopReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }

    MaaTaskId task_id = tasker->post_stop();
    TaskerPostStopReverseResponse resp {
        .task_id = task_id,
    };
    send(resp);

    return true;
}

bool AgentClient::handle_tasker_stopping(const json::value& j)
{
    if (!j.is<TaskerStoppingReverseRequest>()) {
        return false;
    }
    const TaskerStoppingReverseRequest& req = j.as<TaskerStoppingReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }
    bool stopping = tasker->stopping();
    TaskerStoppingReverseResponse resp {
        .ret = stopping,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_tasker_resource(const json::value& j)
{
    if (!j.is<TaskerResourceReverseRequest>()) {
        return false;
    }
    const TaskerResourceReverseRequest& req = j.as<TaskerResourceReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }
    MaaResource* resource = tasker->resource();
    TaskerResourceReverseResponse resp {
        .resource_id = resource_id(resource),
    };
    send(resp);
    return true;
}

bool AgentClient::handle_tasker_controller(const json::value& j)
{
    if (!j.is<TaskerControllerReverseRequest>()) {
        return false;
    }
    const TaskerControllerReverseRequest& req = j.as<TaskerControllerReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }
    MaaController* controller = tasker->controller();
    TaskerControllerReverseResponse resp {
        .controller_id = controller_id(controller),
    };
    send(resp);
    return true;
}

bool AgentClient::handle_tasker_clear_cache(const json::value& j)
{
    if (!j.is<TaskerClearCacheReverseRequest>()) {
        return false;
    }
    const TaskerClearCacheReverseRequest& req = j.as<TaskerClearCacheReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }
    tasker->clear_cache();
    TaskerClearCacheReverseResponse resp {};
    send(resp);
    return true;
}

bool AgentClient::handle_tasker_get_task_detail(const json::value& j)
{
    if (!j.is<TaskerGetTaskDetailReverseRequest>()) {
        return false;
    }
    const TaskerGetTaskDetailReverseRequest& req = j.as<TaskerGetTaskDetailReverseRequest>();
    // LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }
    auto detail_opt = tasker->get_task_detail(req.task_id);
    const auto& detail = detail_opt.value_or(MAA_TASK_NS::TaskDetail {});

    TaskerGetTaskDetailReverseResponse resp {
        .has_value = detail_opt.has_value(),
        .task_id = detail.task_id,
        .entry = detail.entry,
        .node_ids = detail.node_ids,
        .status = static_cast<int32_t>(detail.status),
    };
    send(resp);
    return true;
}

bool AgentClient::handle_tasker_get_node_detail(const json::value& j)
{
    if (!j.is<TaskerGetNodeDetailReverseRequest>()) {
        return false;
    }
    const TaskerGetNodeDetailReverseRequest& req = j.as<TaskerGetNodeDetailReverseRequest>();
    // LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }
    auto detail_opt = tasker->get_node_detail(req.node_id);
    const auto& detail = detail_opt.value_or(MAA_TASK_NS::NodeDetail {});

    TaskerGetNodeDetailReverseResponse resp {
        .has_value = detail_opt.has_value(),
        .node_id = detail.node_id,
        .name = detail.name,
        .reco_id = detail.reco_id,
        .action_id = detail.action_id,
        .completed = detail.completed,
    };
    send(resp);

    return true;
}

bool AgentClient::handle_tasker_get_reco_result(const json::value& j)
{
    if (!j.is<TaskerGetRecoResultReverseRequest>()) {
        return false;
    }
    const TaskerGetRecoResultReverseRequest& req = j.as<TaskerGetRecoResultReverseRequest>();
    // LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }
    auto detail_opt = tasker->get_reco_result(req.reco_id);
    const auto& detail = detail_opt.value_or(MAA_TASK_NS::RecoResult {});

    std::vector<std::string> draws;
    for (const auto& draw : detail.draws) {
        draws.emplace_back(send_image_encoded(draw));
    }

    TaskerGetRecoResultReverseResponse resp {
        .has_value = detail_opt.has_value(),
        .reco_id = detail.reco_id,
        .name = detail.name,
        .algorithm = detail.algorithm,
        .hit = detail.box.has_value(),
        .box = detail.box ? std::array<int32_t, 4> { detail.box->x, detail.box->y, detail.box->width, detail.box->height }
                          : std::array<int32_t, 4> {},
        .detail = detail.detail,
        .raw = send_image_encoded(detail.raw),
        .draws = std::move(draws),
    };
    send(resp);

    return true;
}

bool AgentClient::handle_tasker_get_action_result(const json::value& j)
{
    if (!j.is<TaskerGetActionResultReverseRequest>()) {
        return false;
    }
    const TaskerGetActionResultReverseRequest& req = j.as<TaskerGetActionResultReverseRequest>();
    // LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }
    auto detail_opt = tasker->get_action_result(req.action_id);
    const auto& detail = detail_opt.value_or(MAA_TASK_NS::ActionResult {});

    TaskerGetActionResultReverseResponse resp {
        .has_value = detail_opt.has_value(),
        .action_id = detail.action_id,
        .name = detail.name,
        .action = detail.action,
        .box = std::array<int32_t, 4> { detail.box.x, detail.box.y, detail.box.width, detail.box.height },
        .success = detail.success,
        .detail = detail.detail,
    };
    send(resp);

    return true;
}

bool AgentClient::handle_tasker_get_latest_node(const json::value& j)
{
    if (!j.is<TaskerGetLatestNodeReverseRequest>()) {
        return false;
    }
    const TaskerGetLatestNodeReverseRequest& req = j.as<TaskerGetLatestNodeReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaTasker* tasker = query_tasker(req.tasker_id);
    if (!tasker) {
        LogError << "tasker not found" << VAR(req.tasker_id);
        return false;
    }

    auto node_id_opt = tasker->get_latest_node(req.node_name);

    TaskerGetLatestNodeReverseResponse resp {
        .has_value = node_id_opt.has_value(),
        .latest_id = node_id_opt.value_or(MaaNodeId {}),
    };
    send(resp);

    return true;
}

bool AgentClient::handle_resource_post_bundle(const json::value& j)
{
    if (!j.is<ResourcePostBundleReverseRequest>()) {
        return false;
    }
    const ResourcePostBundleReverseRequest& req = j.as<ResourcePostBundleReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "resource not found" << VAR(req.resource_id);
        return false;
    }

    MaaResId res_id = resource->post_bundle(path(req.path));
    ResourcePostBundleReverseResponse resp {
        .res_id = res_id,
    };
    send(resp);

    return true;
}

bool AgentClient::handle_resource_post_ocr_model(const json::value& j)
{
    if (!j.is<ResourcePostOcrModelReverseRequest>()) {
        return false;
    }
    const ResourcePostOcrModelReverseRequest& req = j.as<ResourcePostOcrModelReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "resource not found" << VAR(req.resource_id);
        return false;
    }

    MaaResId res_id = resource->post_ocr_model(path(req.path));
    ResourcePostOcrModelReverseResponse resp {
        .res_id = res_id,
    };
    send(resp);

    return true;
}

bool AgentClient::handle_resource_post_pipeline(const json::value& j)
{
    if (!j.is<ResourcePostPipelineReverseRequest>()) {
        return false;
    }
    const ResourcePostPipelineReverseRequest& req = j.as<ResourcePostPipelineReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "resource not found" << VAR(req.resource_id);
        return false;
    }

    MaaResId res_id = resource->post_pipeline(path(req.path));
    ResourcePostPipelineReverseResponse resp {
        .res_id = res_id,
    };
    send(resp);

    return true;
}

bool AgentClient::handle_resource_post_image(const json::value& j)
{
    if (!j.is<ResourcePostImageReverseRequest>()) {
        return false;
    }
    const ResourcePostImageReverseRequest& req = j.as<ResourcePostImageReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "resource not found" << VAR(req.resource_id);
        return false;
    }

    MaaResId res_id = resource->post_image(path(req.path));
    ResourcePostImageReverseResponse resp {
        .res_id = res_id,
    };
    send(resp);

    return true;
}

void AgentClient::clear_custom_registration()
{
    LogTrace;

    for (const auto& reco : registered_recognitions_) {
        LogInfo << "unregister pre recognition" << VAR(reco);
        bound_res_->unregister_custom_recognition(reco);
    }
    for (const auto& act : registered_actions_) {
        LogInfo << "unregister pre action" << VAR(act);
        bound_res_->unregister_custom_action(act);
    }

    registered_recognitions_.clear();
    registered_actions_.clear();
}

void AgentClient::clear_resource_sink()
{
    LogTrace;

    if (reg_res_sink_id_ != MaaInvalidId) {
        if (reg_res_) {
            reg_res_->remove_sink(reg_res_sink_id_);
        }
        reg_res_sink_id_ = MaaInvalidId;
    }

    if (reg_res_) {
        reg_res_ = nullptr;
    }
}

void AgentClient::clear_controller_sink()
{
    LogTrace;

    if (reg_ctrl_sink_id_ != MaaInvalidId) {
        if (reg_ctrl_) {
            reg_ctrl_->remove_sink(reg_ctrl_sink_id_);
        }
        reg_ctrl_sink_id_ = MaaInvalidId;
    }

    if (reg_ctrl_) {
        reg_ctrl_ = nullptr;
    }
}

void AgentClient::clear_tasker_sink()
{
    LogTrace;

    if (reg_tasker_sink_id_ != MaaInvalidId) {
        if (reg_tasker_) {
            reg_tasker_->remove_sink(reg_tasker_sink_id_);
        }
        reg_tasker_sink_id_ = MaaInvalidId;
    }

    if (reg_tasker_ && reg_context_sink_id_ != MaaInvalidId) {
        if (reg_tasker_) {
            reg_tasker_->remove_context_sink(reg_context_sink_id_);
        }
        reg_context_sink_id_ = MaaInvalidId;
    }

    if (reg_tasker_) {
        reg_tasker_ = nullptr;
    }
}

bool AgentClient::handle_resource_status(const json::value& j)
{
    if (!j.is<ResourceStatusReverseRequest>()) {
        return false;
    }
    const ResourceStatusReverseRequest& req = j.as<ResourceStatusReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "resource not found" << VAR(req.resource_id);
        return false;
    }
    MaaStatus status = resource->status(req.res_id);
    ResourceStatusReverseResponse resp {
        .status = status,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_resource_wait(const json::value& j)
{
    if (!j.is<ResourceWaitReverseRequest>()) {
        return false;
    }
    const ResourceWaitReverseRequest& req = j.as<ResourceWaitReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "resource not found" << VAR(req.resource_id);
        return false;
    }
    MaaStatus status = resource->wait(req.res_id);
    ResourceWaitReverseResponse resp {
        .status = status,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_resource_valid(const json::value& j)
{
    if (!j.is<ResourceValidReverseRequest>()) {
        return false;
    }
    const ResourceValidReverseRequest& req = j.as<ResourceValidReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "resource not found" << VAR(req.resource_id);
        return false;
    }
    bool valid = resource->valid();
    ResourceValidReverseResponse resp {
        .ret = valid,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_resource_running(const json::value& j)
{
    if (!j.is<ResourceRunningReverseRequest>()) {
        return false;
    }
    const ResourceRunningReverseRequest& req = j.as<ResourceRunningReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "resource not found" << VAR(req.resource_id);
        return false;
    }
    bool running = resource->running();
    ResourceRunningReverseResponse resp {
        .ret = running,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_resource_clear(const json::value& j)
{
    if (!j.is<ResourceClearReverseRequest>()) {
        return false;
    }
    const ResourceClearReverseRequest& req = j.as<ResourceClearReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "resource not found" << VAR(req.resource_id);
        return false;
    }
    bool ret = resource->clear();
    ResourceClearReverseResponse resp {
        .ret = ret,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_resource_override_pipeline(const json::value& j)
{
    if (!j.is<ResourceOverridePipelineReverseRequest>()) {
        return false;
    }

    const ResourceOverridePipelineReverseRequest& req = j.as<ResourceOverridePipelineReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "resource not found" << VAR(req.resource_id);
        return false;
    }

    bool ret = resource->override_pipeline(req.pipeline_override);

    ResourceOverridePipelineReverseResponse resp {
        .ret = ret,
    };
    send(resp);

    return true;
}

bool AgentClient::handle_resource_override_next(const json::value& j)
{
    if (!j.is<ResourceOverrideNextReverseRequest>()) {
        return false;
    }

    const ResourceOverrideNextReverseRequest& req = j.as<ResourceOverrideNextReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "resource not found" << VAR(req.resource_id);
        return false;
    }

    bool ret = resource->override_next(req.node_name, req.next);

    ResourceOverrideNextReverseResponse resp {
        .ret = ret,
    };
    send(resp);

    return true;
}

bool AgentClient::handle_resource_override_image(const json::value& j)
{
    if (!j.is<ResourceOverrideImageReverseRequest>()) {
        return false;
    }

    const ResourceOverrideImageReverseRequest& req = j.as<ResourceOverrideImageReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "resource not found" << VAR(req.resource_id);
        return false;
    }

    cv::Mat image = get_image_cache(req.image);
    bool ret = resource->override_image(req.image_name, image);

    ResourceOverrideImageReverseResponse resp {
        .ret = ret,
    };
    send(resp);

    return true;
}

bool AgentClient::handle_resource_get_node_data(const json::value& j)
{
    if (!j.is<ResourceGetNodeDataReverseRequest>()) {
        return false;
    }

    const ResourceGetNodeDataReverseRequest& req = j.as<ResourceGetNodeDataReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "Resource not found" << VAR(req.resource_id);
        return false;
    }

    auto opt = resource->get_node_data(req.node_name);

    ResourceGetNodeDataReverseResponse resp {
        .has_value = opt.has_value(),
        .node_data = opt ? *opt : json::object(),
    };
    send(resp);

    return true;
}

bool AgentClient::handle_resource_get_hash(const json::value& j)
{
    if (!j.is<ResourceGetHashReverseRequest>()) {
        return false;
    }
    const ResourceGetHashReverseRequest& req = j.as<ResourceGetHashReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "resource not found" << VAR(req.resource_id);
        return false;
    }
    std::string hash = resource->get_hash();
    ResourceGetHashReverseResponse resp {
        .hash = hash,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_resource_get_node_list(const json::value& j)
{
    if (!j.is<ResourceGetNodeListReverseRequest>()) {
        return false;
    }

    const ResourceGetNodeListReverseRequest& req = j.as<ResourceGetNodeListReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "resource not found" << VAR(req.resource_id);
        return false;
    }

    auto node_list = resource->get_node_list();
    ResourceGetNodeListReverseResponse resp {
        .node_list = std::move(node_list),
    };
    send(resp);

    return true;
}

bool AgentClient::handle_resource_get_custom_recognition_list(const json::value& j)
{
    if (!j.is<ResourceGetCustomRecognitionListReverseRequest>()) {
        return false;
    }

    const ResourceGetCustomRecognitionListReverseRequest& req = j.as<ResourceGetCustomRecognitionListReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "resource not found" << VAR(req.resource_id);
        return false;
    }

    auto custom_recognition_list = resource->get_custom_recognition_list();
    ResourceGetCustomRecognitionListReverseResponse resp {
        .custom_recognition_list = std::move(custom_recognition_list),
    };
    send(resp);

    return true;
}

bool AgentClient::handle_resource_get_custom_action_list(const json::value& j)
{
    if (!j.is<ResourceGetCustomActionListReverseRequest>()) {
        return false;
    }

    const ResourceGetCustomActionListReverseRequest& req = j.as<ResourceGetCustomActionListReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);

    MaaResource* resource = query_resource(req.resource_id);
    if (!resource) {
        LogError << "resource not found" << VAR(req.resource_id);
        return false;
    }

    auto custom_action_list = resource->get_custom_action_list();
    ResourceGetCustomActionListReverseResponse resp {
        .custom_action_list = std::move(custom_action_list),
    };
    send(resp);

    return true;
}

bool AgentClient::handle_controller_post_connection(const json::value& j)
{
    if (!j.is<ControllerPostConnectionReverseRequest>()) {
        return false;
    }
    const ControllerPostConnectionReverseRequest& req = j.as<ControllerPostConnectionReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaCtrlId ctrl_id = controller->post_connection();
    ControllerPostConnectionReverseResponse resp {
        .ctrl_id = ctrl_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_post_click(const json::value& j)
{
    if (!j.is<ControllerPostClickReverseRequest>()) {
        return false;
    }
    const ControllerPostClickReverseRequest& req = j.as<ControllerPostClickReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaCtrlId ctrl_id = controller->post_click(req.x, req.y);
    ControllerPostClickReverseResponse resp {
        .ctrl_id = ctrl_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_post_swipe(const json::value& j)
{
    if (!j.is<ControllerPostSwipeReverseRequest>()) {
        return false;
    }
    const ControllerPostSwipeReverseRequest& req = j.as<ControllerPostSwipeReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaCtrlId ctrl_id = controller->post_swipe(req.x1, req.y1, req.x2, req.y2, req.duration);
    ControllerPostSwipeReverseResponse resp {
        .ctrl_id = ctrl_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_post_click_key(const json::value& j)
{
    if (!j.is<ControllerPostClickKeyReverseRequest>()) {
        return false;
    }
    const ControllerPostClickKeyReverseRequest& req = j.as<ControllerPostClickKeyReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaCtrlId ctrl_id = controller->post_click_key(req.keycode);
    ControllerPostClickKeyReverseResponse resp {
        .ctrl_id = ctrl_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_post_input_text(const json::value& j)
{
    if (!j.is<ControllerPostInputTextReverseRequest>()) {
        return false;
    }
    const ControllerPostInputTextReverseRequest& req = j.as<ControllerPostInputTextReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaCtrlId ctrl_id = controller->post_input_text(req.text);
    ControllerPostInputTextReverseResponse resp {
        .ctrl_id = ctrl_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_post_start_app(const json::value& j)
{
    if (!j.is<ControllerPostStartAppReverseRequest>()) {
        return false;
    }
    const ControllerPostStartAppReverseRequest& req = j.as<ControllerPostStartAppReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaCtrlId ctrl_id = controller->post_start_app(req.intent);
    ControllerPostStartAppReverseResponse resp {
        .ctrl_id = ctrl_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_post_stop_app(const json::value& j)
{
    if (!j.is<ControllerPostStopAppReverseRequest>()) {
        return false;
    }
    const ControllerPostStopAppReverseRequest& req = j.as<ControllerPostStopAppReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaCtrlId ctrl_id = controller->post_stop_app(req.intent);
    ControllerPostStopAppReverseResponse resp {
        .ctrl_id = ctrl_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_post_screencap(const json::value& j)
{
    if (!j.is<ControllerPostScreencapReverseRequest>()) {
        return false;
    }
    const ControllerPostScreencapReverseRequest& req = j.as<ControllerPostScreencapReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaCtrlId ctrl_id = controller->post_screencap();
    ControllerPostScreencapReverseResponse resp {
        .ctrl_id = ctrl_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_post_shell(const json::value& j)
{
    if (!j.is<ControllerPostShellReverseRequest>()) {
        return false;
    }
    const ControllerPostShellReverseRequest& req = j.as<ControllerPostShellReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaCtrlId ctrl_id = controller->post_shell(req.cmd, req.timeout);
    ControllerPostShellReverseResponse resp {
        .ctrl_id = ctrl_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_post_touch_down(const json::value& j)
{
    if (!j.is<ControllerPostTouchDownReverseRequest>()) {
        return false;
    }
    const ControllerPostTouchDownReverseRequest& req = j.as<ControllerPostTouchDownReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaCtrlId ctrl_id = controller->post_touch_down(req.contact, req.x, req.y, req.pressure);
    ControllerPostTouchDownReverseResponse resp {
        .ctrl_id = ctrl_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_post_touch_move(const json::value& j)
{
    if (!j.is<ControllerPostTouchMoveReverseRequest>()) {
        return false;
    }
    const ControllerPostTouchMoveReverseRequest& req = j.as<ControllerPostTouchMoveReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaCtrlId ctrl_id = controller->post_touch_move(req.contact, req.x, req.y, req.pressure);
    ControllerPostTouchMoveReverseResponse resp {
        .ctrl_id = ctrl_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_post_touch_up(const json::value& j)
{
    if (!j.is<ControllerPostTouchUpReverseRequest>()) {
        return false;
    }
    const ControllerPostTouchUpReverseRequest& req = j.as<ControllerPostTouchUpReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaCtrlId ctrl_id = controller->post_touch_up(req.contact);
    ControllerPostTouchUpReverseResponse resp {
        .ctrl_id = ctrl_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_post_key_down(const json::value& j)
{
    if (!j.is<ControllerPostKeyDownReverseRequest>()) {
        return false;
    }
    const ControllerPostKeyDownReverseRequest& req = j.as<ControllerPostKeyDownReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaCtrlId ctrl_id = controller->post_key_down(req.keycode);
    ControllerPostKeyDownReverseResponse resp {
        .ctrl_id = ctrl_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_post_key_up(const json::value& j)
{
    if (!j.is<ControllerPostKeyUpReverseRequest>()) {
        return false;
    }
    const ControllerPostKeyUpReverseRequest& req = j.as<ControllerPostKeyUpReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaCtrlId ctrl_id = controller->post_key_up(req.keycode);
    ControllerPostKeyUpReverseResponse resp {
        .ctrl_id = ctrl_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_post_scroll(const json::value& j)
{
    if (!j.is<ControllerPostScrollReverseRequest>()) {
        return false;
    }
    const ControllerPostScrollReverseRequest& req = j.as<ControllerPostScrollReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaCtrlId ctrl_id = controller->post_scroll(req.dx, req.dy);
    ControllerPostScrollReverseResponse resp {
        .ctrl_id = ctrl_id,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_status(const json::value& j)
{
    if (!j.is<ControllerStatusReverseRequest>()) {
        return false;
    }
    const ControllerStatusReverseRequest& req = j.as<ControllerStatusReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaStatus status = controller->status(req.ctrl_id);
    ControllerStatusReverseResponse resp {
        .status = status,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_wait(const json::value& j)
{
    if (!j.is<ControllerWaitReverseRequest>()) {
        return false;
    }
    const ControllerWaitReverseRequest& req = j.as<ControllerWaitReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    MaaStatus status = controller->wait(req.ctrl_id);
    ControllerWaitReverseResponse resp {
        .status = status,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_connected(const json::value& j)
{
    if (!j.is<ControllerConnectedReverseRequest>()) {
        return false;
    }
    const ControllerConnectedReverseRequest& req = j.as<ControllerConnectedReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    bool connected = controller->connected();
    ControllerConnectedReverseResponse resp {
        .ret = connected,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_running(const json::value& j)
{
    if (!j.is<ControllerRunningReverseRequest>()) {
        return false;
    }
    const ControllerRunningReverseRequest& req = j.as<ControllerRunningReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    bool running = controller->running();
    ControllerRunningReverseResponse resp {
        .ret = running,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_cached_image(const json::value& j)
{
    if (!j.is<ControllerCachedImageReverseRequest>()) {
        return false;
    }
    const ControllerCachedImageReverseRequest& req = j.as<ControllerCachedImageReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    auto image = controller->cached_image();
    ControllerCachedImageReverseResponse resp {
        .image = send_image(image),
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_get_shell_output(const json::value& j)
{
    if (!j.is<ControllerGetShellOutputReverseRequest>()) {
        return false;
    }
    const ControllerGetShellOutputReverseRequest& req = j.as<ControllerGetShellOutputReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    std::string output = controller->cached_shell_output();
    ControllerGetShellOutputReverseResponse resp {
        .output = output,
    };
    send(resp);
    return true;
}

bool AgentClient::handle_controller_get_uuid(const json::value& j)
{
    if (!j.is<ControllerGetUuidReverseRequest>()) {
        return false;
    }
    const ControllerGetUuidReverseRequest& req = j.as<ControllerGetUuidReverseRequest>();
    LogFunc << VAR(req) << VAR(ipc_addr_);
    MaaController* controller = query_controller(req.controller_id);
    if (!controller) {
        LogError << "controller not found" << VAR(req.controller_id);
        return false;
    }
    std::string uuid = controller->get_uuid();
    ControllerGetUuidReverseResponse resp {
        .uuid = uuid,
    };
    send(resp);
    return true;
}

// FIXME: 不应该存在这个函数
// 等 send_and_recv 支持在子线程中使用后，应该删除这个函数
bool AgentClient::handle_event_response(const json::value& j)
{
    if (j.is<ResourceEventResponse>() || j.is<ControllerEventResponse>() || j.is<TaskerEventResponse>()) {
        LogTrace << "handle event response" << VAR(j);
        return true;
    }
    return false;
}

MaaBool AgentClient::reco_agent(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_recognition_name,
    const char* custom_recognition_param,
    const MaaImageBuffer* image,
    const MaaRect* roi,
    void* trans_arg,
    MaaRect* out_box,
    MaaStringBuffer* out_detail)
{
    LogTrace << VAR_VOIDP(context) << VAR(task_id) << VAR(node_name) << VAR(custom_recognition_name) << VAR(custom_recognition_param);

    if (!trans_arg) {
        LogError << "trans_arg is null";
        return false;
    }

    AgentClient* pthis = reinterpret_cast<AgentClient*>(trans_arg);
    if (!pthis) {
        LogError << "pthis is null";
        return false;
    }

    if (!image) {
        LogError << "image is null";
        return false;
    }

    if (!pthis->alive()) {
        LogError << "server is not alive" << VAR(pthis->ipc_addr_);
        return false;
    }

    const cv::Mat& mat = image->get();

    CustomRecognitionRequest req {
        .context_id = pthis->context_id(context),
        .task_id = task_id,
        .node_name = node_name,
        .custom_recognition_name = custom_recognition_name,
        .custom_recognition_param = custom_recognition_param,
        .image = pthis->send_image(mat),
        .roi = roi ? std::array<int32_t, 4> { roi->x, roi->y, roi->width, roi->height } : std::array<int32_t, 4> {},
    };

    auto resp_opt = pthis->send_and_recv<CustomRecognitionResponse>(req);

    if (!resp_opt) {
        LogError << "failed to send_and_recv" << VAR(req);
        return false;
    }
    const CustomRecognitionResponse& resp = *resp_opt;
    LogTrace << VAR(resp);

    if (out_box) {
        *out_box = MaaRect { resp.out_box[0], resp.out_box[1], resp.out_box[2], resp.out_box[3] };
    }
    if (out_detail) {
        out_detail->set(resp.out_detail);
    }
    return resp.ret;
}

MaaBool AgentClient::action_agent(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_action_name,
    const char* custom_action_param,
    MaaRecoId reco_id,
    const MaaRect* box,
    void* trans_arg)
{
    LogTrace << VAR_VOIDP(context) << VAR(task_id) << VAR(node_name) << VAR(custom_action_name) << VAR(custom_action_param);

    if (!trans_arg) {
        LogError << "trans_arg is null";
        return false;
    }

    AgentClient* pthis = reinterpret_cast<AgentClient*>(trans_arg);
    if (!pthis) {
        LogError << "pthis is null";
        return false;
    }

    CustomActionRequest req {
        .context_id = pthis->context_id(context),
        .task_id = task_id,
        .node_name = node_name,
        .custom_action_name = custom_action_name,
        .custom_action_param = custom_action_param,
        .reco_id = reco_id,
        .box = box ? std::array<int32_t, 4> { box->x, box->y, box->width, box->height } : std::array<int32_t, 4> {},
    };

    auto resp_opt = pthis->send_and_recv<CustomActionResponse>(req);
    if (!resp_opt) {
        LogError << "failed to send_and_recv" << VAR(req);
        return false;
    }

    const CustomActionResponse& resp = *resp_opt;
    LogTrace << VAR(resp);

    return resp.ret;
}

std::string AgentClient::context_id(MaaContext* context)
{
    std::stringstream ss;
    ss << context;
    std::string id = std::move(ss).str();

    context_map_.insert_or_assign(id, context);
    return id;
}

MaaContext* AgentClient::query_context(const std::string& context_id)
{
    auto it = context_map_.find(context_id);
    if (it == context_map_.end()) {
        LogError << "context not found" << VAR(context_id);
        return nullptr;
    }
    return it->second;
}

std::string AgentClient::tasker_id(MaaTasker* tasker)
{
    std::stringstream ss;
    ss << tasker;
    std::string id = std::move(ss).str();

    tasker_map_.insert_or_assign(id, tasker);
    return id;
}

MaaTasker* AgentClient::query_tasker(const std::string& tasker_id)
{
    auto it = tasker_map_.find(tasker_id);
    if (it == tasker_map_.end()) {
        LogError << "tasker not found" << VAR(tasker_id);
        return nullptr;
    }
    return it->second;
}

std::string AgentClient::controller_id(MaaController* controller)
{
    std::stringstream ss;
    ss << controller;
    std::string id = std::move(ss).str();

    controller_map_.insert_or_assign(id, controller);
    return id;
}

MaaController* AgentClient::query_controller(const std::string& controller_id)
{
    auto it = controller_map_.find(controller_id);
    if (it == controller_map_.end()) {
        LogError << "controller not found" << VAR(controller_id);
        return nullptr;
    }
    return it->second;
}

std::string AgentClient::resource_id(MaaResource* resource)
{
    std::stringstream ss;
    ss << resource;
    std::string id = std::move(ss).str();

    resource_map_.insert_or_assign(id, resource);
    return id;
}

MaaResource* AgentClient::query_resource(const std::string& resource_id)
{
    auto it = resource_map_.find(resource_id);
    if (it == resource_map_.end()) {
        LogError << "resource not found" << VAR(resource_id);
        return nullptr;
    }
    return it->second;
}

void AgentClient::res_event_sink(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    LogTrace << VAR_VOIDP(handle) << VAR(message) << VAR(details_json) << VAR_VOIDP(trans_arg);

    if (!trans_arg) {
        LogError << "trans_arg is null";
        return;
    }

    AgentClient* pthis = reinterpret_cast<AgentClient*>(trans_arg);
    if (!pthis) {
        LogError << "pthis is null";
        return;
    }

    if (!pthis->alive()) {
        LogError << "server is not alive" << VAR(pthis->ipc_addr_);
        return;
    }

    ResourceEventRequest req {
        .resource_id = pthis->resource_id(reinterpret_cast<MaaResource*>(handle)),
        .message = message,
        .details = json::parse(details_json).value_or(json::value {}),
    };

    // FIXME: 应该用 send_and_recv 但是暂时用 send 代替
    // 由于 sink 在 async_runner 的子线程中的，recv 会被其他线程抢掉。
    // 一直接收不到 recv 导致卡死在这里
    // std::ignore = pthis->send_and_recv<ControllerEventResponse>(req);
    std::ignore = pthis->send(req);
}

void AgentClient::ctrl_event_sink(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    LogTrace << VAR_VOIDP(handle) << VAR(message) << VAR(details_json) << VAR_VOIDP(trans_arg);

    if (!trans_arg) {
        LogError << "trans_arg is null";
        return;
    }

    AgentClient* pthis = reinterpret_cast<AgentClient*>(trans_arg);
    if (!pthis) {
        LogError << "pthis is null";
        return;
    }

    if (!pthis->alive()) {
        LogError << "server is not alive" << VAR(pthis->ipc_addr_);
        return;
    }

    ControllerEventRequest req {
        .controller_id = pthis->controller_id(reinterpret_cast<MaaController*>(handle)),
        .message = message,
        .details = json::parse(details_json).value_or(json::value {}),
    };

    // FIXME: 应该用 send_and_recv 但是暂时用 send 代替
    // 由于 sink 在 async_runner 的子线程中的，recv 会被其他线程抢掉。
    // 一直接收不到 recv 导致卡死在这里
    // std::ignore = pthis->send_and_recv<ControllerEventResponse>(req);
    std::ignore = pthis->send(req);
}

void AgentClient::tasker_event_sink(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    LogTrace << VAR_VOIDP(handle) << VAR(message) << VAR(details_json) << VAR_VOIDP(trans_arg);

    if (!trans_arg) {
        LogError << "trans_arg is null";
        return;
    }

    AgentClient* pthis = reinterpret_cast<AgentClient*>(trans_arg);
    if (!pthis) {
        LogError << "pthis is null";
        return;
    }

    if (!pthis->alive()) {
        LogError << "server is not alive" << VAR(pthis->ipc_addr_);
        return;
    }

    TaskerEventRequest req {
        .tasker_id = pthis->tasker_id(reinterpret_cast<MaaTasker*>(handle)),
        .message = message,
        .details = json::parse(details_json).value_or(json::value {}),
    };

    // FIXME: 应该用 send_and_recv 但是暂时用 send 代替
    // 由于 sink 在 async_runner 的子线程中的，recv 会被其他线程抢掉。
    // 一直接收不到 recv 导致卡死在这里
    // std::ignore = pthis->send_and_recv<ControllerEventResponse>(req);
    std::ignore = pthis->send(req);
}

void AgentClient::ctx_event_sink(void* handle, const char* message, const char* details_json, void* trans_arg)
{
    LogTrace << VAR_VOIDP(handle) << VAR(message) << VAR(details_json) << VAR_VOIDP(trans_arg);

    if (!trans_arg) {
        LogError << "trans_arg is null";
        return;
    }

    AgentClient* pthis = reinterpret_cast<AgentClient*>(trans_arg);
    if (!pthis) {
        LogError << "pthis is null";
        return;
    }

    if (!pthis->alive()) {
        LogError << "server is not alive" << VAR(pthis->ipc_addr_);
        return;
    }

    ContextEventRequest req {
        .context_id = pthis->context_id(reinterpret_cast<MaaContext*>(handle)),
        .message = message,
        .details = json::parse(details_json).value_or(json::value {}),
    };

    std::ignore = pthis->send_and_recv<ContextEventResponse>(req);
}

MAA_AGENT_CLIENT_NS_END
