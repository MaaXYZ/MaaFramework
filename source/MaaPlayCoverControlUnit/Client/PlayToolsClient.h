#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class PlayToolsClient
{
public:
    enum class TouchPhase : uint8_t
    {
        Began = 0,
        Moved = 1,
        Ended = 3,
    };

    PlayToolsClient();
    ~PlayToolsClient();

    PlayToolsClient(const PlayToolsClient&) = delete;
    PlayToolsClient& operator=(const PlayToolsClient&) = delete;

    bool connect(const std::string& address);
    void close();
    bool is_connected() const;

    bool screencap(std::vector<uint8_t>& buffer, int& width, int& height);
    bool touch(TouchPhase phase, int x, int y);
    bool terminate();

    std::pair<int, int> screen_size() const;

private:
    bool open();
    bool check_version();
    bool fetch_screen_size(int& width, int& height);

    static constexpr int MinimalVersion = 2;

    boost::asio::io_context context_;
    boost::asio::ip::tcp::socket socket_;
    std::string address_;
    std::pair<int, int> screen_size_ { 0, 0 };
};

MAA_CTRL_UNIT_NS_END
