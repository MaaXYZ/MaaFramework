#include "AgentClient.h"

#include <sstream>

#include <cpp-base64/base64.hpp>
#include <meojson/json.hpp>

#include "Common/MaaTypes.h"
#include "MaaAgent/Message.hpp"
#include "MaaFramework/MaaAPI.h"
#include "Utils/Buffer/ImageBuffer.hpp"
#include "Utils/Buffer/StringBuffer.hpp"
#include "Utils/Codec.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_AGENT_CLIENT_NS_BEGIN

#ifdef _WIN32
std::vector<std::wstring> conv_args(const std::vector<std::string>& args)
{
    std::vector<std::wstring> wargs;
    for (const auto& arg : args) {
        wargs.emplace_back(to_u16(arg));
    }
    return wargs;
}
#else
std::vector<std::string> conv_args(const std::vector<std::string>& args)
{
    return args;
}
#endif

AgentClient::AgentClient(MaaNotificationCallback notify, void* notify_trans_arg)
    : notifier_(notify, notify_trans_arg)
{
    LogFunc << VAR_VOIDP(notify) << VAR_VOIDP(notify_trans_arg);
}

AgentClient::~AgentClient()
{
    LogFunc;

    send_and_recv<ShutDownResponse>(ShutDownRequest {});

    if (child_ && child_.joinable()) {
        child_.join();
    }
}

bool AgentClient::bind_resource(MaaResource* resource)
{
    LogInfo << VAR_VOIDP(this) << VAR_VOIDP(resource);

    if (resource_) {
        LogError << "resource is already bound, if you want to rebind, please re-create MaaAgent";
        return false;
    }

    resource_ = resource;

    return true;
}

bool AgentClient::start_clild(const std::filesystem::path& child_exec, const std::vector<std::string>& child_args)
{
    LogFunc << VAR(child_exec) << VAR(child_args);

    if (!resource_) {
        LogError << "resource is not bound, please bind resource first";
        return false;
    }

    if (child_) {
        LogError << "child is already running, if you want to restart, please re-create MaaAgent";
        return false;
    }

    std::filesystem::path exec = boost::process::search_path(child_exec);
    if (!std::filesystem::exists(exec)) {
        LogError << "exec not found" << VAR(child_exec);
        return false;
    }

    std::string addr = create_socket();

    std::vector<std::string> args = child_args;
    args.emplace_back(addr);

    child_ = boost::process::child(exec, conv_args(args));
    if (!child_) {
        LogError << "failed to start child" << VAR(exec) << VAR(child_args);
        return false;
    }

    bool connected = recv_and_handle_start_up_response();
    if (!connected) {
        LogError << "failed to recv_and_handle_start_up_response";
        return false;
    }

    return true;
}

std::string AgentClient::create_socket()
{
    constexpr std::string_view kAddrFormat = "ipc://maafw-agent-{}";
    std::stringstream ss;
    ss << resource_;
    ipc_addr_ = std::format(kAddrFormat, std::move(ss).str());
    LogInfo << VAR(ipc_addr_);

    child_sock_ = zmq::socket_t(child_ctx_, zmq::socket_type::pair);
    child_sock_.bind(ipc_addr_);

    return ipc_addr_;
}

bool AgentClient::send(const json::value& j)
{
    LogTrace << VAR(j) << VAR(ipc_addr_);

    std::string jstr = j.dumps();
    zmq::message_t msg(jstr.size());
    std::memcpy(msg.data(), jstr.data(), jstr.size());
    bool sent = child_sock_.send(msg, zmq::send_flags::none).has_value();
    if (!sent) {
        LogError << "failed to send msg" << VAR(j);
        return false;
    }
    return true;
}

std::optional<json::value> AgentClient::recv()
{
    LogFunc << VAR(ipc_addr_);

    zmq::message_t msg;
    auto size_opt = child_sock_.recv(msg);
    if (!size_opt || *size_opt == 0) {
        LogError << "failed to recv msg";
        return std::nullopt;
    }

    std::string_view str = msg.to_string_view();
    LogTrace << VAR(str);

    auto jopt = json::parse(str);
    if (!jopt) {
        LogError << "failed to parse msg";
        return std::nullopt;
    }

    return *jopt;
}

bool AgentClient::recv_and_handle_start_up_response()
{
    LogFunc << VAR(ipc_addr_);

    if (!resource_) {
        LogError << "resource is not bound";
        return false;
    }

    auto msg_opt = recv<StartUpResponse>();
    if (!msg_opt) {
        LogError << "failed to recv StartUpResponse";
        return false;
    }

    const StartUpResponse& msg = *msg_opt;
    LogInfo << VAR(msg);

    for (const auto& reco : msg.recognitions) {
        LogTrace << VAR(reco);
        resource_->register_custom_recognition(reco, reco_agent, this);
    }
    for (const auto& act : msg.actions) {
        LogTrace << VAR(act);
        resource_->register_custom_action(act, action_agent, this);
    }

    return true;
}

bool AgentClient::handle_reverse_request(const json::value& j)
{
    LogFunc << VAR(j) << VAR(ipc_addr_);

    if (handle_context_run_task(j)) {
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

    auto task_id = MaaContextRunTask(context, req.entry.c_str(), req.pipeline_override.c_str());

    ContextRunTaskReverseResponse resp {
        .task_id = task_id,
    };
    send(resp);

    return true;
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

    CustomRecognitionRequest req {
        .context_id = pthis->context_id(context),
        .task_id = task_id,
        .node_name = node_name,
        .custom_recognition_name = custom_recognition_name,
        .custom_recognition_param = custom_recognition_param,
        .image = encode_image(image),
        .roi = roi ? std::array<int32_t, 4> { roi->x, roi->y, roi->width, roi->height } : std::array<int32_t, 4> {},
    };

    auto reverse_request_handler = [pthis](const json::value& j) {
        return pthis->handle_reverse_request(j);
    };

    auto resp_opt = pthis->send_and_recv<CustomRecognitionResponse>(req, reverse_request_handler);

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

    auto reverse_request_handler = [pthis](const json::value& j) {
        return pthis->handle_reverse_request(j);
    };

    auto resp_opt = pthis->send_and_recv<CustomActionResponse>(req, reverse_request_handler);
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

std::string AgentClient::encode_image(const MaaImageBuffer* image)
{
    if (!image) {
        LogError << "image is null";
        return {};
    }

    cv::Mat mat = image->get();
    std::vector<uint8_t> buf;
    cv::imencode(".png", mat, buf);

    return base64::base64_encode(buf.data(), buf.size());
}

MAA_AGENT_CLIENT_NS_END
