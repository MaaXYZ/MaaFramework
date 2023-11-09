#include "UnitBase.h"

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

void UnitBase::set_io(std::shared_ptr<PlatformIO> io_ptr)
{
    for (auto child : children_) {
        child->set_io(io_ptr);
    }
    io_ptr_ = io_ptr;
}

void UnitBase::set_replacement(Argv::replacement argv_replace)
{
    for (auto child : children_) {
        child->set_replacement(argv_replace);
    }
    argv_replace_ = std::move(argv_replace);
}

void UnitBase::merge_replacement(Argv::replacement argv_replace, bool _override)
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

bool UnitBase::parse_argv(const std::string& key, const json::value& config, Argv& argv)
{
    auto aopt = config.find<json::object>("command");
    if (!aopt) {
        LogError << "Cannot find argv entry";
        return false;
    }

    auto opt = aopt->find<json::value>(key);
    if (!opt) {
        LogError << "Cannot find key" << VAR(key);
        return false;
    }

    if (!argv.parse(*opt)) {
        LogError << "Parse config failed:" << VAR(key);
        return false;
    }

    return true;
}

std::optional<std::string> UnitBase::command(const Argv::value& cmd, bool recv_by_socket, int64_t timeout)
{
    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return std::nullopt;
    }

    auto start_time = std::chrono::steady_clock::now();

    std::string pipe_data;
    std::string sock_data;
    int ret = io_ptr_->call_command(cmd, recv_by_socket, pipe_data, sock_data, timeout);

    auto duration = duration_since(start_time);

    std::string scmd = json::array(cmd).to_string();
    LogDebug << VAR(scmd) << VAR(ret) << VAR(pipe_data.size()) << VAR(sock_data.size()) << VAR(duration);

    if (!pipe_data.empty() && pipe_data.size() < 4096) {
        LogDebug << MAA_LOG_NS::separator::newline << "stdout output:" << pipe_data;
    }
    if (recv_by_socket && !sock_data.empty() && sock_data.size() < 4096) {
        LogDebug << MAA_LOG_NS::separator::newline << "socket output:" << sock_data;
    }

    if (ret != 0) {
        LogError << "call_command failed" << VAR(cmd) << VAR(ret);
        return std::nullopt;
    }

    return recv_by_socket ? sock_data : pipe_data;
}

MAA_CTRL_UNIT_NS_END
