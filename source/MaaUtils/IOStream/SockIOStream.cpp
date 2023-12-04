#include "SockIOStream.h"

#include "SockIOStream.h"
#include "Utils/Logger.h"

MAA_NS_BEGIN

ServerSockIOFactory::ServerSockIOFactory(const std::string& address, unsigned short port) : server_acceptor_(io_ctx_)
{
    using namespace boost::asio::ip;

    tcp::endpoint endpoint(address::from_string(address), port);

    server_acceptor_.open(endpoint.protocol());
    server_acceptor_.set_option(tcp::acceptor::reuse_address(true));
    server_acceptor_.bind(endpoint);

    server_acceptor_.listen();
}

ServerSockIOFactory::~ServerSockIOFactory()
{
    server_acceptor_.close();
}

unsigned short ServerSockIOFactory::port() const
{
    return server_acceptor_.local_endpoint().port();
}

std::shared_ptr<SockIOStream> ServerSockIOFactory::accept()
{
    auto sock = server_acceptor_.accept();
    if (!sock.is_open()) {
        LogError << "socket is not opened";
        return nullptr;
    }

    return std::make_shared<SockIOStream>(std::move(sock));
}

ClientSockIOFactory::ClientSockIOFactory(const std::string& address, unsigned short port)
    : endpoint_(boost::asio::ip::address::from_string(address), port)
{}

std::shared_ptr<SockIOStream> ClientSockIOFactory::connect()
{
    boost::asio::ip::tcp::socket sock(io_ctx_);
    sock.connect(endpoint_);
    if (!sock.is_open()) {
        LogError << "socket is not opened";
        return nullptr;
    }

    return std::make_shared<SockIOStream>(std::move(sock));
}

SockIOStream::SockIOStream(boost::asio::ip::tcp::socket&& sock)
    : sock_(std::move(sock)), buffer_(std::make_unique<char[]>(kBufferSize))
{}

SockIOStream::~SockIOStream()
{
    sock_.close();
}

bool SockIOStream::write(std::string_view data)
{
    if (!sock_.is_open()) {
        return false;
    }

    sock_.write_some(boost::asio::buffer(data));
    return true;
}

std::string SockIOStream::read(std::chrono::milliseconds timeout, size_t count)
{
    auto start_time = std::chrono::steady_clock::now();
    auto check_timeout = [&](const auto& start_time) -> bool {
        return std::chrono::steady_clock::now() - start_time < timeout;
    };

    std::string result;

    while (check_timeout(start_time) && count > result.size() && sock_.is_open()) {
        auto read_size = std::min(kBufferSize, count - result.size());
        auto read_num = sock_.read_some(boost::asio::mutable_buffer(buffer_.get(), read_size));
        if (read_num > 0) {
            result.append(buffer_.get(), read_num);
        }
    }

    return result;
}

MAA_NS_END
