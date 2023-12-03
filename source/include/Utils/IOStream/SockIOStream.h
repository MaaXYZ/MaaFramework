#pragma once

#include <memory>

#include "MaaFramework/MaaPort.h"
#include "Utils/Boost.hpp"
#include "Utils/NonCopyable.hpp"

MAA_NS_BEGIN

class MAA_UTILS_API SockIOStream;

class MAA_UTILS_API ServerSockIOFactory : public NonCopyButMovable
{
public:
    ServerSockIOFactory(const std::string& address, unsigned short port);
    ~ServerSockIOFactory();

    unsigned short port() const;

public:
    std::shared_ptr<SockIOStream> accept();

private:
    boost::asio::io_context io_ctx_;
    boost::asio::ip::tcp::acceptor server_acceptor_;
};

class MAA_UTILS_API ClientSockIOFactory : public NonCopyButMovable
{
public:
    ClientSockIOFactory(const std::string& address, unsigned short port);
    ~ClientSockIOFactory() = default;

public:
    std::shared_ptr<SockIOStream> connect();

private:
    boost::asio::io_context io_ctx_;
    boost::asio::ip::tcp::endpoint endpoint_;
};

class MAA_UTILS_API SockIOStream : public NonCopyButMovable
{
    static constexpr size_t kBufferSize = 128 * 1024;

public:
    SockIOStream(boost::asio::ip::tcp::socket&& sock);
    ~SockIOStream();

public:
    bool write(std::string_view data);
    std::string read(std::chrono::seconds timeout, size_t count = SIZE_MAX);

private:
    boost::asio::ip::tcp::socket sock_;

    std::unique_ptr<char[]> buffer_ = nullptr;
};

MAA_NS_END
