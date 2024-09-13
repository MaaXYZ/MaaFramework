#include "Utils/IOStream/SockIOStream.h"

#include "Utils/Logger.h"

MAA_NS_BEGIN

ServerSockIOFactory::ServerSockIOFactory(const std::string& address, unsigned short port)
    : server_acceptor_(io_ctx_)
{
    LogFunc << VAR(address) << VAR(port);

    using namespace boost::asio::ip;

    tcp::endpoint endpoint(address::from_string(address), port);

    server_acceptor_.open(endpoint.protocol());
    server_acceptor_.set_option(tcp::acceptor::reuse_address(true));
    server_acceptor_.bind(endpoint);

    server_acceptor_.listen();
}

ServerSockIOFactory::~ServerSockIOFactory()
{
    LogFunc;

    server_acceptor_.close();
}

uint16_t ServerSockIOFactory::port() const
{
    return server_acceptor_.local_endpoint().port();
}

std::shared_ptr<SockIOStream> ServerSockIOFactory::accept()
{
    // LogFunc;

    boost::asio::ip::tcp::iostream ios;
    auto accept_future = std::async(std::launch::async, [&]() { server_acceptor_.accept(*ios.rdbuf()); });
    if (accept_future.wait_for(std::chrono::seconds(2)) != std::future_status::ready) {
        LogError << "accept timeout";
        server_acceptor_.cancel();
        return nullptr;
    }

    if (ios.eof()) {
        LogError << "socket is not opened";
        return nullptr;
    }

    return std::make_shared<SockIOStream>(std::move(ios));
}

ClientSockIOFactory::ClientSockIOFactory(const std::string& address, unsigned short port)
    : endpoint_(boost::asio::ip::address::from_string(address), port)
{
    LogFunc << VAR(address) << VAR(port);
}

std::shared_ptr<SockIOStream> ClientSockIOFactory::connect()
{
    LogFunc;

    boost::asio::ip::tcp::iostream ios(endpoint_);
    if (ios.eof()) {
        LogError << "socket is not opened";
        return nullptr;
    }
    return std::make_shared<SockIOStream>(std::move(ios));
}

SockIOStream::SockIOStream(boost::asio::ip::tcp::iostream&& ios)
    : ios_(std::move(ios))
{
}

SockIOStream::~SockIOStream()
{
    release();
}

bool SockIOStream::write(std::string_view data)
{
    if (!ios_.good()) {
        LogError << "ios is not good";
        return false;
    }

    ios_ << data << std::endl;
    return true;
}

bool SockIOStream::release()
{
    ios_.close();
    return true;
}

bool SockIOStream::is_open() const
{
    return !ios_.eof();
}

void SockIOStream::expires_after(duration_t timeout)
{
    ios_.expires_after(timeout);
}

std::string SockIOStream::read_once(size_t max_count)
{
    constexpr size_t kBufferSize = 128 * 1024;

    if (!buffer_) {
        buffer_ = std::make_unique<char[]>(kBufferSize);
    }

    size_t count = std::min(kBufferSize, max_count);
    auto read = ios_.read(buffer_.get(), count).gcount();
    return std::string(buffer_.get(), read);
}

MAA_NS_END
