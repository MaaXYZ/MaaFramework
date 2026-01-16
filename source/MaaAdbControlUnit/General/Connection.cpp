#include "Connection.h"

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

Connection::Connection(std::filesystem::path adb_path, std::string adb_serial)
    : adb_path_(std::move(adb_path))
    , adb_serial_(std::move(adb_serial))
{
}

Connection::~Connection()
{
    stop_monitor();
}

bool Connection::parse(const json::value& config)
{
    static const json::array kDefaultConnectArgv = {
        "{ADB}",
        "connect",
        "{ADB_SERIAL}",
    };
    static const json::array kDefaultKillServerArgv = {
        "{ADB}",
        "kill-server",
    };
    static const json::array kDefaultTestConnectionArgv = {
        "{ADB}",
        "-s",
        "{ADB_SERIAL}",
        "get-state",
    };
    // 使用 cat 等待 stdin，最轻量的长期挂起命令
    static const json::array kDefaultMonitorArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "cat",
    };

    return parse_command("Connect", config, kDefaultConnectArgv, connect_argv_)
           && parse_command("KillServer", config, kDefaultKillServerArgv, kill_server_argv_)
           && parse_command("TestConnection", config, kDefaultTestConnectionArgv, test_connection_argv_)
           && parse_command("Monitor", config, kDefaultMonitorArgv, monitor_argv_);
}

bool Connection::connect()
{
    LogFunc;

    bool is_remote = adb_serial_.find(':') != std::string::npos;

    if (is_remote) {
        if (!connect_remote()) {
            LogInfo << "failed to connect remote";
            return false;
        }
    }

    if (!test_connection()) {
        return false;
    }

    start_monitor();

    return true;
}

bool Connection::kill_server()
{
    LogFunc;

    auto argv_opt = kill_server_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return false;
    }

    using namespace std::chrono_literals;
    return startup_and_read_pipe(*argv_opt, 60s).has_value();
}

bool Connection::test_connection()
{
    LogFunc;

    auto argv_opt = test_connection_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return false;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return false;
    }

    // get-state 返回 "device" 表示设备正常连接，"offline" 或 "bootloader" 表示不可用
    return output_opt->find("device") != std::string::npos;
}

bool Connection::connect_remote()
{
    auto argv_opt = connect_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return false;
    }

    using namespace std::chrono_literals;
    auto output_opt = startup_and_read_pipe(*argv_opt, 60s);
    if (!output_opt) {
        return false;
    }

    constexpr std::array<std::string_view, 4> kErrorFlag = { "error", "cannot", "refused", "unable to connect" };
    for (const auto& flag : kErrorFlag) {
        if (output_opt->find(flag) != std::string::npos) {
            LogInfo << "unable to connect";
            return false;
        }
    }

    LogInfo << "connected";
    return true;
}

bool Connection::start_monitor()
{
    LogFunc;

    stop_monitor();

    auto argv_opt = monitor_argv_.gen(argv_replace_);
    if (!argv_opt) {
        LogError << "failed to generate monitor argv";
        return false;
    }

    monitor_ios_ = std::make_shared<ChildPipeIOStream>(argv_opt->exec, argv_opt->args);

    LogInfo << "monitor started";
    return true;
}

void Connection::stop_monitor()
{
    if (monitor_ios_) {
        LogFunc;
        monitor_ios_->release();
        monitor_ios_ = nullptr;
    }
}

bool Connection::is_alive() const
{
    if (!monitor_ios_) {
        return false;
    }
    return monitor_ios_->running();
}

MAA_CTRL_UNIT_NS_END
