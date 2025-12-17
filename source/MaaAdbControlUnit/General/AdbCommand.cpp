#include "AdbCommand.h"

#include <ranges>

#include "MaaUtils/Logger.h"
#include "MaaUtils/StringMisc.hpp"

MAA_CTRL_UNIT_NS_BEGIN

bool AdbCommand::parse(const json::value& config)
{
    static const json::array kDefaultShellArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "{CMD}",
    };

    return parse_command("Shell", config, kDefaultShellArgv, shell_argv);
}

std::optional<std::string> AdbCommand::shell(const std::string& cmd, std::chrono::milliseconds timeout)
{
    LogFunc << VAR(cmd) << VAR(timeout);

    merge_replacement({ { "{CMD}", cmd } });

    auto argv_opt = shell_argv.gen(argv_replace_);
    if (!argv_opt) {
        return std::nullopt;
    }

    return startup_and_read_pipe(*argv_opt, timeout);
}

MAA_CTRL_UNIT_NS_END
