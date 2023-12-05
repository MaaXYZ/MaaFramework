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

std::string SockIOStream::read(duration_t timeout)
{
    return read_some(std::numeric_limits<size_t>::max(), timeout);
}

std::string SockIOStream::read_some(size_t count, duration_t timeout)
{
    auto start_time = std::chrono::steady_clock::now();
    std::string result;

    while (sock_.is_open() && result.size() < count && duration_since(start_time) < timeout) {
        auto read_size = std::min(kBufferSize, count - result.size());
        auto read_num = sock_.read_some(boost::asio::mutable_buffer(buffer_.get(), read_size));
        result.append(buffer_.get(), read_num);
    }

    return result;
}

std::string SockIOStream::read_until(std::string_view delimiter, duration_t timeout)
{
    auto start_time = std::chrono::steady_clock::now();

    std::string result;

    while (!result.ends_with(delimiter)) {
        auto sub_timeout = timeout - duration_since<duration_t>(start_time);
        if (sub_timeout < duration_t::zero()) {
            break;
        }

        auto sub_str = read_some(1, sub_timeout);
        result.append(std::move(sub_str));
    }

    return result;
}

MAA_NS_END
