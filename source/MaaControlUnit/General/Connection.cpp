#include "Connection.h"

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

bool Connection::parse(const json::value& config)
{
    return parse_argv("Connect", config, connect_argv_) && parse_argv("KillServer", config, kill_server_argv_);
}

bool Connection::connect()
{
    LogFunc;

    auto cmd_ret = command(connect_argv_.gen(argv_replace_), false, 60LL * 1000);

    if (!cmd_ret) {
        LogInfo << "run command failed";
        return false;
    }

    constexpr std::array<std::string_view, 4> kErrorFlag = { "error", "cannot", "refused", "unable to connect" };
    for (const auto& flag : kErrorFlag) {
        if (cmd_ret->find(flag) != std::string::npos) {
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

    return command(kill_server_argv_.gen(argv_replace_), false, 60LL * 1000).has_value();
}

MAA_CTRL_UNIT_NS_END
