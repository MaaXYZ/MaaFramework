#pragma once

#include "Utils/Boost.hpp"
#include "Utils/SingletonHolder.hpp"

#include <string>
#include <thread>

MAA_TOOLKIT_NS_BEGIN

class WsServer : public SingletonHolder<WsServer>
{
public:
    friend class SingletonHolder<WsServer>;

    bool start(std::string_view ip, uint16_t port);
    bool stop();

private:
    WsServer() = default;

    bool stopping = false;

    boost::asio::io_context ctx;
    std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor = nullptr;

    std::shared_ptr<std::thread> dispatcher = nullptr;
};

MAA_TOOLKIT_NS_END
