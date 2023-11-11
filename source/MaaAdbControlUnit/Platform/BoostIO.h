#pragma once

#include <functional>

#include "PlatformIO.h"
#include "Utils/Boost.hpp"
#include "Utils/NonCopyable.hpp"
#include "Utils/SingletonHolder.hpp"

MAA_CTRL_UNIT_NS_BEGIN

class BoostIO : public PlatformIO
{
public:
    BoostIO();
    virtual ~BoostIO() override;

    virtual int call_command(const std::vector<std::string>& cmd, bool recv_by_socket, std::string& pipe_data,
                             std::string& sock_data, int64_t timeout) override;

    virtual std::optional<unsigned short> create_socket(const std::string& local_address) override;
    virtual void close_socket() noexcept override;

    virtual std::shared_ptr<IOHandler> tcp(const std::string& target, unsigned short port) override;
    virtual std::shared_ptr<IOHandler> interactive_shell(const std::vector<std::string>& cmd,
                                                         bool want_stderr) override;

private:
    void read_sock_data(std::string& data, std::function<bool(void)> terminate);
    void read_pipe_data(boost::process::ipstream& pout, std::string& data, std::function<bool(void)> terminate);

    std::shared_ptr<boost::asio::io_context> ios_;
    boost::asio::ip::tcp::acceptor server_sock_;
};

class IOHandlerBoostSocket : public IOHandler, NonCopyable
{
public:
    IOHandlerBoostSocket(std::shared_ptr<boost::asio::io_context> ios, boost::asio::ip::tcp::socket&& socket)
        : ios_(ios), sock_(std::move(socket))
    {}

    virtual ~IOHandlerBoostSocket() override;

    virtual bool write(std::string_view data) override;
    virtual std::string read(unsigned timeout_sec) override;
    virtual std::string read(unsigned timeout_sec, size_t expect) override;

private:
    std::shared_ptr<boost::asio::io_context> ios_;
    boost::asio::ip::tcp::socket sock_;
};

class IOHandlerBoostStream : public IOHandler, NonCopyable
{
public:
    IOHandlerBoostStream(std::shared_ptr<boost::process::ipstream> out, std::shared_ptr<boost::process::opstream> in,
                         std::shared_ptr<boost::process::child> proc)
        : out_(out), in_(in), proc_(proc)
    {}

    virtual ~IOHandlerBoostStream() override;

    virtual bool write(std::string_view data) override;
    virtual std::string read(unsigned timeout_sec) override;
    virtual std::string read(unsigned timeout_sec, size_t expect) override;

private:
    std::shared_ptr<boost::process::ipstream> out_;
    std::shared_ptr<boost::process::opstream> in_;
    std::shared_ptr<boost::process::child> proc_;
};

MAA_CTRL_UNIT_NS_END
