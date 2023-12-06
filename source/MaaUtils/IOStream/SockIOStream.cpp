#include "Utils/IOStream/SockIOStream.h"

#include "Utils/Logger.h"

MAA_NS_BEGIN

ServerSockIOFactory::ServerSockIOFactory(const std::string& address, unsigned short port) : server_acceptor_(io_ctx_)
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

unsigned short ServerSockIOFactory::port() const
{
    return server_acceptor_.local_endpoint().port();
}

std::shared_ptr<SockIOStream> ServerSockIOFactory::accept()
{
    LogFunc;

    auto sock = server_acceptor_.accept();
    if (!sock.is_open()) {
        LogError << "socket is not opened";
        return nullptr;
    }

    return std::make_shared<SockIOStream>(std::move(sock));
}

ClientSockIOFactory::ClientSockIOFactory(const std::string& address, unsigned short port)
    : endpoint_(boost::asio::ip::address::from_string(address), port)
{
    LogFunc << VAR(address) << VAR(port);
}

std::shared_ptr<SockIOStream> ClientSockIOFactory::connect()
{
    LogFunc;

    boost::asio::ip::tcp::socket sock(io_ctx_);
    sock.connect(endpoint_);
    if (!sock.is_open()) {
        LogError << "socket is not opened";
        return nullptr;
    }

    return std::make_shared<SockIOStream>(std::move(sock));
}

SockIOStream::SockIOStream(boost::asio::ip::tcp::socket&& sock) : sock_(std::move(sock)) {}

SockIOStream::~SockIOStream() {}

bool SockIOStream::write(std::string_view data)
{
    if (!is_open()) {
        LogError << "not opened";
        return false;
    }

    sock_.write_some(boost::asio::buffer(data));
    return true;
}

bool SockIOStream::release()
{
    sock_.close();
    return true;
}

bool SockIOStream::is_open() const
{
    return sock_.is_open();
}

std::string SockIOStream::read_once(size_t max_count)
{
    constexpr size_t kBufferSize = 128 * 1024;

    if (!buffer_) {
        buffer_ = std::make_unique<char[]>(kBufferSize);
    }

    auto read_size = std::min(kBufferSize, max_count);
    auto read_num = sock_.read_some(boost::asio::mutable_buffer(buffer_.get(), read_size));
    return std::string(buffer_.get(), read_num);
}

MAA_NS_END
