#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "Common/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "MaaUtils/IOStream/BoostIO.hpp"
#include "MaaUtils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class ProjectInterfaceMgr : public SingletonHolder<ProjectInterfaceMgr>
{
    friend class SingletonHolder<ProjectInterfaceMgr>;

public:
    struct AgentConfig
    {
        std::string child_exec;
        std::vector<std::string> child_args;
        std::string identifier;
    };

public:
    // 加载 interface.json，只读取 agent 字段
    bool load(const std::filesystem::path& interface_path);

    bool loaded() const { return loaded_; }

    const std::filesystem::path& project_dir() const { return project_dir_; }

    const AgentConfig& agent_config() const { return agent_config_; }

    // 绑定资源
    bool bind_resource(MaaResource* resource);

    // 启动 Agent（使用已加载的配置）
    bool start_agent();

    // 停止 Agent
    void stop_agent();

    // 检查 Agent 是否正在运行
    bool agent_running();

    // 检查 Agent 连接状态
    bool agent_connected() const;

private:
    ProjectInterfaceMgr() = default;
    ~ProjectInterfaceMgr();

    void cleanup_agent();

private:
    bool loaded_ = false;
    std::filesystem::path project_dir_;
    AgentConfig agent_config_;

    MaaAgentClient* agent_ = nullptr;
    MaaResource* resource_ = nullptr;
    std::optional<boost::process::child> agent_child_;
};

MAA_TOOLKIT_NS_END

