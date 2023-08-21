#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>

#include "Conf/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class IOHandler;

class PlatformIO
{
public:
    virtual ~PlatformIO() = default;

    virtual int call_command(const std::vector<std::string>& cmd, bool recv_by_socket, std::string& pipe_data,
                             std::string& sock_data, int64_t timeout) = 0;

    virtual std::optional<unsigned short> create_socket(const std::string& local_address) = 0;
    virtual void close_socket() noexcept = 0;

    virtual std::shared_ptr<IOHandler> tcp(const std::string& target, unsigned short port) = 0;
    virtual std::shared_ptr<IOHandler> interactive_shell(const std::vector<std::string>& cmd, bool want_stderr) = 0;

    bool support_socket_ = false;
};

class IOHandler
{
public:
    virtual ~IOHandler() = default;

    virtual bool write(std::string_view data) = 0;
    virtual std::string read(unsigned timeout_sec) = 0;
    virtual std::string read(unsigned timeout_sec, size_t expect) = 0;
};

MAA_CTRL_UNIT_NS_END
