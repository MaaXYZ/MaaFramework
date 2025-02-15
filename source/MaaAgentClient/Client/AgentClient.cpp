#include "AgentClient.h"

#include <sstream>

#include <meojson/json.hpp>

#include "Common/MaaTypes.h"
#include "MaaAgent/Message.hpp"
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

    if (child_ && child_.joinable()) {
        child_.join();
    }
}

bool AgentClient::bind_resource(MaaResource* resource)
{
    LogInfo << VAR_VOIDP(this) << VAR_VOIDP(resource);

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
        LogError << "child is already running";
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

    return recv_and_handle_init_msg();
}

std::string AgentClient::create_socket()
{
    constexpr std::string_view kAddrFormat = "ipc://maafw-agent-{}";
    std::stringstream ss;
    ss << resource_;
    std::string addr = std::format(kAddrFormat, std::move(ss).str());
    LogInfo << VAR(addr);

    child_sock_ = zmq::socket_t(child_ctx_, zmq::socket_type::pair);
    child_sock_.bind(addr);

    return addr;
}

std::optional<json::value> AgentClient::recv()
{
    LogFunc;

    zmq::message_t init_msg;
    auto size_opt = child_sock_.recv(init_msg);
    if (!size_opt || *size_opt == 0) {
        LogError << "failed to recv init_msg";
        return std::nullopt;
    }

    const std::string& init_str = init_msg.to_string();
    LogInfo << VAR(init_str);

    auto jopt = json::parse(init_str);
    if (!jopt) {
        LogError << "failed to parse init_msg";
        return std::nullopt;
    }

    return *jopt;
}

bool AgentClient::recv_and_handle_init_msg()
{
    LogFunc;

    if (!resource_) {
        LogError << "resource is not bound";
        return false;
    }

    auto msg_opt = recv<InitMsg>();
    if (!msg_opt) {
        LogError << "failed to recv init_msg";
        return false;
    }

    const InitMsg& msg = *msg_opt;
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
    if (!trans_arg) {
        LogError << "trans_arg is null";
        return false;
    }

    AgentClient* pthis = reinterpret_cast<AgentClient*>(trans_arg);
    if (!pthis) {
        LogError << "pthis is null";
        return false;
    }

    return true;
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
    if (!trans_arg) {
        LogError << "trans_arg is null";
        return false;
    }

    AgentClient* pthis = reinterpret_cast<AgentClient*>(trans_arg);
    if (!pthis) {
        LogError << "pthis is null";
        return false;
    }

    return true;
}

MAA_AGENT_CLIENT_NS_END
