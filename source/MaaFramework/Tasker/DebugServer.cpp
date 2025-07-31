#include "DebugServer.h"

#include <boost/asio.hpp>
#include <condition_variable>

#include "Utils/Logger.h"

namespace asio = boost::asio;

MAA_NS_BEGIN

class DebugServerImpl : public DebugServer
{
public:
    virtual bool start_service() override;

    virtual std::string call_remote(std::string request) override;

private:
    asio::awaitable<void> main_routine();

    asio::awaitable<void> setup_udp_broadcast();

    asio::io_context io_context_;

    asio::ip::tcp::endpoint tcp_endpoint_ { asio::ip::make_address("127.0.0.1"), 0 };
    asio::ip::udp::endpoint udp_endpoint_ { {}, 13111 };

    asio::ip::tcp::acceptor acceptor_ { io_context_ };
    asio::ip::tcp::socket tcp_socket_ { io_context_ };

    asio::ip::udp::socket udp_socket_ { io_context_ };

    bool connected_ { false };
    std::condition_variable tcp_connected_;
    std::mutex tcp_connected_lock_;
};

std::shared_ptr<DebugServer> DebugServer::create_server()
{
    return std::shared_ptr<DebugServer>(new DebugServerImpl());
}

bool DebugServerImpl::start_service()
{
    LogFunc;

    asio::co_spawn(io_context_, [this]() -> asio::awaitable<void> { co_await main_routine(); }, asio::detached);

    std::thread io_thread([this]() { io_context_.run(); });
    io_thread.detach();

    LogInfo << "Start waiting connection";
    std::unique_lock lock(tcp_connected_lock_);
    tcp_connected_.wait(lock, [this]() { return connected_; });

    return true;
}

std::string DebugServerImpl::call_remote(std::string request)
{
    LogInfo << "Send " << request;
    asio::write(tcp_socket_, asio::buffer(request + "\n"));

    asio::streambuf response_buf;
    asio::read_until(tcp_socket_, response_buf, "\n");

    auto buffer = response_buf.data();
    auto response = std::string {
        asio::buffers_begin(buffer),
        asio::buffers_begin(buffer),
    };
    LogInfo << "Recv " << response;

    return response;
}

asio::awaitable<void> DebugServerImpl::main_routine()
{
    LogFunc;

    acceptor_.open(tcp_endpoint_.protocol());
    acceptor_.set_option(asio::socket_base::reuse_address(true));
    acceptor_.bind(tcp_endpoint_);
    acceptor_.listen();

    asio::co_spawn(io_context_, [this]() -> asio::awaitable<void> { co_await setup_udp_broadcast(); }, asio::detached);

    LogInfo << "Tcp socket listening";
    co_await acceptor_.async_accept(tcp_socket_, asio::use_awaitable);
    LogInfo << "Tcp socket accepted";

    udp_socket_.close();

    connected_ = true;
    tcp_connected_.notify_one();
}

asio::awaitable<void> DebugServerImpl::setup_udp_broadcast()
{
    LogFunc;

    udp_socket_.open(udp_endpoint_.protocol());
    udp_socket_.set_option(asio::socket_base::reuse_address(true));
    udp_socket_.bind(udp_endpoint_);

    while (true) {
        char data[1024];
        asio::ip::udp::endpoint sender_endpoint;
        std::size_t length = co_await udp_socket_.async_receive_from(asio::buffer(data), sender_endpoint, asio::use_awaitable);

        std::string msg(data, length);
        LogInfo << "Recv udp broadcast " << msg;
        if (msg == "MaaFramework.DiscoverDebugServer") {
            std::string reply = std::to_string(acceptor_.local_endpoint().port());
            co_await udp_socket_.async_send_to(asio::buffer(reply), sender_endpoint, asio::use_awaitable);
        }
    }
}

MAA_NS_END
