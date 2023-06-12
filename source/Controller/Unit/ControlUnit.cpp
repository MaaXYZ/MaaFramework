#include "ControlUnit.h"

#include "Utils/Logger.hpp"
#include "Utils/StringMisc.hpp"

MAA_CTRL_UNIT_NS_BEGIN

void UnitHelper::set_io(std::shared_ptr<PlatformIO> io_ptr)
{
    io_ptr_ = std::move(io_ptr);
}

void UnitHelper::set_replacement(Argv::replacement argv_replace)
{
    argv_replace_ = std::move(argv_replace);
}

void UnitHelper::merge_replacement(Argv::replacement argv_replace, bool _override)
{
    if (_override) {
        argv_replace.merge(argv_replace_);
        argv_replace_ = std::move(argv_replace);
    }
    else {
        argv_replace_.merge(argv_replace);
    }
}

bool UnitHelper::parse_argv(const std::string& key, const json::value& config, Argv& argv)
{
    auto opt = config.find<json::value>(key);
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

std::optional<std::string> UnitHelper::command(Argv::value cmd, bool recv_by_socket, int64_t timeout)
{
    if (!io_ptr_) {
        LogError << "io_ptr is nullptr";
        return std::nullopt;
    }

    auto start_time = std::chrono::steady_clock::now();

    std::string pipe_data;
    std::string sock_data;
    int ret = io_ptr_->call_command(cmd, recv_by_socket, pipe_data, sock_data, timeout);

    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count();

    LogInfo << VAR(cmd) << VAR(ret) << VAR(pipe_data.size()) << VAR(sock_data.size()) << VAR(duration);
    if (!pipe_data.empty() && pipe_data.size() < 4096) {
        LogTrace << Logger::separator::newline << "stdout output:" << pipe_data;
    }
    if (recv_by_socket && !sock_data.empty() && sock_data.size() < 4096) {
        LogTrace << Logger::separator::newline << "socket output:" << sock_data;
    }

    if (ret != 0) {
        LogError << "call_command failed" << VAR(cmd) << VAR(ret);
        return std::nullopt;
    }

    return recv_by_socket ? sock_data : pipe_data;
}

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

bool DeviceInfo::parse(const json::value& config)
{
    return parse_argv("Uuid", config, uuid_argv_) && parse_argv("Resolution", config, resolution_argv_) &&
           parse_argv("Orientation", config, orientation_argv_);
}

bool DeviceInfo::uuid(std::string& uuid)
{
    LogFunc;

    auto cmd_ret = command(uuid_argv_.gen(argv_replace_));

    if (!cmd_ret.has_value()) {
        return false;
    }

    auto uuid_str = cmd_ret.value();
    std::erase_if(uuid_str, [](char c) { return !std::isdigit(c) && !std::isalpha(c); });
    uuid = std::move(uuid_str);

    return true;
}

bool DeviceInfo::resolution(int& width, int& height)
{
    LogFunc;

    auto cmd_ret = command(resolution_argv_.gen(argv_replace_));

    if (!cmd_ret.has_value()) {
        return false;
    }

    std::istringstream iss(cmd_ret.value());
    int s1, s2;
    iss >> s1 >> s2;

    width = std::max(s1, s2);
    height = std::min(s1, s2);

    return true;
}

bool DeviceInfo::orientation(int& ori)
{
    LogFunc;

    auto cmd_ret = command(orientation_argv_.gen(argv_replace_));

    if (!cmd_ret.has_value()) {
        return false;
    }

    const auto& s = cmd_ret.value();

    if (s.empty()) {
        return false;
    }

    auto o = s.front() - '0';

    if (!(o >= 0 && o <= 3)) {
        return false;
    }

    ori = o;

    return true;
}

bool Activity::parse(const json::value& config)
{
    return false;
}

bool TapInput::parse(const json::value& config)
{
    return false;
}

bool Screencap::parse(const json::value& config)
{
    return false;
}

bool InvokeApp::parse(const json::value& config)
{
    return false;
}

MAA_CTRL_UNIT_NS_END
