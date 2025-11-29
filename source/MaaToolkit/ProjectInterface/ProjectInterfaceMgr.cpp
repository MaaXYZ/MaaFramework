#include "ProjectInterfaceMgr.h"

#include <meojson/json.hpp>

#include "MaaAgentClient/MaaAgentClientAPI.h"
#include "MaaFramework/MaaAPI.h"
#include "MaaUtils/Encoding.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/StringMisc.hpp"

MAA_TOOLKIT_NS_BEGIN

#ifdef _WIN32
static std::vector<std::wstring> conv_args(const std::vector<std::string>& args)
{
    std::vector<std::wstring> wargs;
    for (const auto& arg : args) {
        wargs.emplace_back(to_u16(arg));
    }
    return wargs;
}
#else
static std::vector<std::string> conv_args(const std::vector<std::string>& args)
{
    return args;
}
#endif

ProjectInterfaceMgr::~ProjectInterfaceMgr()
{
    stop_agent();
}

bool ProjectInterfaceMgr::load(const std::filesystem::path& interface_path)
{
    LogFunc << VAR(interface_path);

    auto json_opt = json::open(interface_path, true, true);
    if (!json_opt) {
        LogError << "failed to parse" << interface_path;
        return false;
    }

    const json::value& json = *json_opt;

    // 只读取 agent 字段
    auto agent_opt = json.find("agent");
    if (!agent_opt || !agent_opt->is_object()) {
        LogWarn << "agent field not found or not an object";
        // 没有 agent 也可以算成功
        loaded_ = true;
        project_dir_ = interface_path.parent_path();
        return true;
    }

    const auto& agent = *agent_opt;

    agent_config_.child_exec = agent.get("child_exec", std::string {});
    agent_config_.identifier = agent.get("identifier", std::string {});

    auto args_opt = agent.find("child_args");
    if (args_opt && args_opt->is_array()) {
        for (const auto& arg : args_opt->as_array()) {
            if (arg.is_string()) {
                agent_config_.child_args.emplace_back(arg.as_string());
            }
        }
    }

    project_dir_ = interface_path.parent_path();
    loaded_ = true;

    LogInfo << "Agent config loaded" << VAR(agent_config_.child_exec) << VAR(agent_config_.child_args);
    return true;
}

bool ProjectInterfaceMgr::bind_resource(MaaResource* resource)
{
    LogFunc << VAR(resource);

    if (!resource) {
        LogError << "resource is null";
        return false;
    }

    resource_ = resource;
    return true;
}

bool ProjectInterfaceMgr::start_agent()
{
    LogFunc;

    if (!loaded_) {
        LogError << "interface not loaded";
        return false;
    }

    if (!resource_) {
        LogError << "resource not bound";
        return false;
    }

    if (agent_config_.child_exec.empty()) {
        LogError << "agent child_exec is empty";
        return false;
    }

    // 处理 {PROJECT_DIR} 占位符
    constexpr std::string_view kProjectDir = "{PROJECT_DIR}";
    std::string project_dir = path_to_utf8_string(project_dir_);

    std::filesystem::path child_exec = path(string_replace_all(agent_config_.child_exec, kProjectDir, project_dir));

    std::vector<std::string> child_args;
    for (const auto& arg : agent_config_.child_args) {
        child_args.emplace_back(string_replace_all(arg, kProjectDir, project_dir));
    }

    // 创建 AgentClient
    MaaStringBuffer* id_buffer = nullptr;
    if (!agent_config_.identifier.empty()) {
        id_buffer = MaaStringBufferCreate();
        MaaStringBufferSetEx(id_buffer, agent_config_.identifier.c_str(), agent_config_.identifier.size());
    }

    agent_ = MaaAgentClientCreateV2(id_buffer);
    if (id_buffer) {
        MaaStringBufferDestroy(id_buffer);
    }

    if (!agent_) {
        LogError << "Failed to create agent client";
        return false;
    }

    // 绑定资源
    if (!MaaAgentClientBindResource(agent_, resource_)) {
        LogError << "Failed to bind resource";
        cleanup_agent();
        return false;
    }

    // 获取 socket id
    auto* socket_id_buffer = MaaStringBufferCreate();
    if (!MaaAgentClientIdentifier(agent_, socket_id_buffer)) {
        LogError << "Failed to get socket id";
        MaaStringBufferDestroy(socket_id_buffer);
        cleanup_agent();
        return false;
    }
    std::string socket_id = MaaStringBufferGet(socket_id_buffer);
    MaaStringBufferDestroy(socket_id_buffer);

    // 准备参数
    std::vector<std::string> args = child_args;
    args.emplace_back(socket_id);

    // 查找可执行文件
    std::filesystem::path exec = child_exec;
    if (!std::filesystem::exists(exec)) {
        exec = boost::process::search_path(child_exec);
    }
    if (exec.empty() || !std::filesystem::exists(exec)) {
        LogError << "Failed to find agent executable" << VAR(child_exec);
        cleanup_agent();
        return false;
    }

    auto os_args = conv_args(args);
    LogInfo << "Starting Agent" << VAR(exec) << VAR(os_args);

    try {
        agent_child_ = boost::process::child(exec, os_args);
    }
    catch (const std::exception& e) {
        LogError << "Failed to start agent process" << VAR(e.what());
        cleanup_agent();
        return false;
    }

    // 连接 Agent
    if (!MaaAgentClientConnect(agent_)) {
        LogError << "Failed to connect agent";
        cleanup_agent();
        return false;
    }

    LogInfo << "Agent started and connected successfully";
    return true;
}

void ProjectInterfaceMgr::stop_agent()
{
    LogFunc;

    if (agent_) {
        MaaAgentClientDisconnect(agent_);
    }

    cleanup_agent();
}

bool ProjectInterfaceMgr::agent_running()
{
    return agent_child_.has_value() && agent_child_->running();
}

bool ProjectInterfaceMgr::agent_connected() const
{
    return agent_ && MaaAgentClientConnected(agent_);
}

void ProjectInterfaceMgr::cleanup_agent()
{
    LogFunc;

    if (agent_child_.has_value()) {
        try {
            if (agent_child_->running()) {
                agent_child_->terminate();
            }
            agent_child_->wait();
        }
        catch (const std::exception& e) {
            LogWarn << "Exception while terminating agent child" << VAR(e.what());
        }
        agent_child_.reset();
    }

    if (agent_) {
        MaaAgentClientDestroy(agent_);
        agent_ = nullptr;
    }
}

MAA_TOOLKIT_NS_END

