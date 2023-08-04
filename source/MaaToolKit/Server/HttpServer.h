#pragma once

#include <string>
#include <thread>

#include "Conf/Conf.h"
#include "Utils/Boost.hpp"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_SERVER_NS_BEGIN

class HttpServer : public SingletonHolder<HttpServer>
{
public:
    friend class SingletonHolder<HttpServer>;

    bool start(std::string_view ip, uint16_t port);
    bool stop();

private:
    HttpServer() = default;

    bool stopping = false;

    boost::asio::io_context ctx;
    std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor = nullptr;

    std::shared_ptr<std::thread> dispatcher = nullptr;
};

MAA_TOOLKIT_SERVER_NS_END
