#include "Connection.h"

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

Connection::Connection(std::filesystem::path adb_path, std::string adb_serial)
    : adb_path_(std::move(adb_path))
    , adb_serial_(std::move(adb_serial))
{
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
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "echo Hello MaaFw!",
    };

    return parse_command("Connect", config, kDefaultConnectArgv, connect_argv_)
           && parse_command("KillServer", config, kDefaultKillServerArgv, kill_server_argv_)
           && parse_command("TestConnection", config, kDefaultTestConnectionArgv, test_connection_argv_);
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

    return test_connection();
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

    return startup_and_read_pipe(*argv_opt).has_value();
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

MAA_CTRL_UNIT_NS_END
