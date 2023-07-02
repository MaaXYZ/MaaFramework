#include "Connection.h"

#include "MaaUtils/Logger.hpp"

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
        return false;
    }

    if (cmd_ret->find("error") != std::string::npos || cmd_ret->find("cannot") != std::string::npos) {
        return false;
    }

    return true;
}

bool Connection::kill_server()
{
    LogFunc;

    return command(kill_server_argv_.gen(argv_replace_), false, 60LL * 1000).has_value();
}

MAA_CTRL_UNIT_NS_END
