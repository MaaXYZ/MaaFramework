#include "UnitBase.h"

#include "MaaUtils/IOStream/ChildPipeIOStream.h"
#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

void UnitBase::set_replacement(Replacement argv_replace)
{
    for (auto child : children_) {
        child->set_replacement(argv_replace);
    }
    argv_replace_ = std::move(argv_replace);
}

void UnitBase::merge_replacement(Replacement argv_replace, bool _override)
{
    for (auto child : children_) {
        child->merge_replacement(argv_replace, _override);
    }
    if (_override) {
        argv_replace.merge(argv_replace_);
        argv_replace_ = std::move(argv_replace);
    }
    else {
        argv_replace_.merge(argv_replace);
    }
}

bool UnitBase::parse_command(
    const std::string& key,
    const json::value& config,
    const json::array& default_argv,
    /*out*/ ProcessArgvGenerator& argv)
{
    auto jargv = config.get("command", key, default_argv);

    auto opt = ProcessArgvGenerator::create(jargv);
    if (!opt) {
        LogError << "Parse config failed:" << VAR(key);
        return false;
    }
    argv = std::move(*opt);

    return true;
}

std::optional<std::string> UnitBase::startup_and_read_pipe(const ProcessArgv& argv, std::chrono::milliseconds timeout)
{
    auto start_time = std::chrono::steady_clock::now();

    ChildPipeIOStream ios(argv.exec, argv.args);
    std::string output = ios.read(timeout);
    bool ret = ios.release();

    auto duration = duration_since(start_time);

    LogDebug << VAR(output.size()) << VAR(duration);
    if (!output.empty() && output.size() < 4096) {
        LogDebug << MAA_LOG_NS::separator::newline << "output:" << output;
    }

    if (!ret) {
        LogError << "child return error" << VAR(argv.exec) << VAR(argv.args);
        return std::nullopt;
    }

    return output;
}

MAA_CTRL_UNIT_NS_END
