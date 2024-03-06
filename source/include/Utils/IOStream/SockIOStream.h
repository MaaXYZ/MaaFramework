#pragma once

#include <memory>

#include "IOStream.h"
#include "MaaFramework/MaaPort.h"
#include "Utils/NonCopyable.hpp"

MAA_NS_BEGIN

class MAA_UTILS_API SockIOStream;

class MAA_UTILS_API ServerSockIOFactory : public NonCopyButMovable
{
public:
    ServerSockIOFactory(const std::string& address, uint16_t port);
    ~ServerSockIOFactory();

    uint16_t port() const;

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

class MAA_UTILS_API SockIOStream : public IOStream
{
public:
    explicit SockIOStream(boost::asio::ip::tcp::iostream&& ios);

    // NonCopyButMovable
    // https://stackoverflow.com/questions/29289956/c11-virtual-destructors-and-auto-generation-of-move-special-functions
    SockIOStream(const SockIOStream&) = delete;
    SockIOStream(SockIOStream&&) = default;
    SockIOStream& operator=(const SockIOStream&) = delete;
    SockIOStream& operator=(SockIOStream&&) = default;

    virtual ~SockIOStream() override;

public:
    virtual bool write(std::string_view data) override;

    virtual bool release() override;
    virtual bool is_open() const override;

    void expires_after(duration_t timeout);

protected:
    virtual std::string read_once(size_t max_count) override;

private:
    boost::asio::ip::tcp::iostream ios_;

    std::unique_ptr<char[]> buffer_ = nullptr;
};

MAA_NS_END
