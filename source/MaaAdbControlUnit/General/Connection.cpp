#include "Connection.h"

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

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

    return parse_argv("Connect", config, kDefaultConnectArgv, connect_argv_)
           && parse_argv("KillServer", config, kDefaultKillServerArgv, kill_server_argv_);
}

bool Connection::connect()
{
    LogFunc;

    auto argv_opt = connect_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return false;
    }

    using namespace std::chrono_literals;
    auto output_opt = startup_and_read_pipe(*argv_opt, 60s);
    if (!output_opt) {
        return false;
    }

    constexpr std::array<std::string_view, 4> kErrorFlag = { "error",
                                                             "cannot",
                                                             "refused",
                                                             "unable to connect" };
    for (const auto& flag : kErrorFlag) {
        if (output_opt->find(flag) != std::string::npos) {
            LogInfo << "unable to connect";
            return false;
        }
    }

    LogInfo << "connected";
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

MAA_CTRL_UNIT_NS_END