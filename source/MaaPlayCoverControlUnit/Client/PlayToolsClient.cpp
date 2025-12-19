#include "PlayToolsClient.h"

#include <array>
#include <boost/asio.hpp>
#include <cstring>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "MaaUtils/Logger.h"

using boost::asio::ip::tcp;

MAA_CTRL_UNIT_NS_BEGIN

PlayToolsClient::PlayToolsClient()
    : socket_(context_)
{
}

PlayToolsClient::~PlayToolsClient()
{
    close();
}

bool PlayToolsClient::connect(const std::string& address)
{
    if (address_ != address) {
        close();
        address_ = address;
    }

    return open();
}

void PlayToolsClient::close()
{
    screen_size_ = { 0, 0 };

    if (!socket_.is_open()) {
        return;
    }

    boost::system::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_both, ec);
    if (ec && ec != boost::asio::error::not_connected) {
        LogWarn << "Error during socket shutdown:" << ec.message();
    }

    ec.clear();
    socket_.close(ec);
    if (ec) {
        LogWarn << "Error during socket close:" << ec.message();
    }
}

bool PlayToolsClient::is_connected() const
{
    return socket_.is_open() && screen_size_.first > 0;
}

std::pair<int, int> PlayToolsClient::screen_size() const
{
    return screen_size_;
}

bool PlayToolsClient::open()
{
    if (socket_.is_open()) {
        return true;
    }

    std::string host, port;
    std::stringstream ss(address_);
    std::getline(ss, host, ':');
    std::getline(ss, port);

    if (host.empty() || port.empty()) {
        LogError << "Invalid address format:" << address_;
        return false;
    }

    tcp::resolver resolver(context_);

    std::array<uint8_t, 4> buffer;
    constexpr char handshake[4] = { 'M', 'A', 'A', 0 };
    constexpr char signature[4] = { 'O', 'K', 'A', 'Y' };

    boost::system::error_code ec;
    auto endpoints = resolver.resolve(host, port, ec);
    if (ec) {
        LogError << "Cannot resolve address" << address_ << ec.message();
        return false;
    }
    boost::asio::connect(socket_, endpoints, ec);
    if (ec) {
        LogError << "Cannot connect to" << address_ << ec.message();
        return false;
    }
    boost::asio::write(socket_, boost::asio::buffer(handshake), ec);
    if (ec) {
        LogError << "Cannot send handshake:" << ec.message();
        return false;
    }
    boost::asio::read(socket_, boost::asio::buffer(buffer, 4), ec);
    if (ec) {
        LogError << "Cannot read handshake response:" << ec.message();
        return false;
    }

    if (std::memcmp(buffer.data(), signature, 4) != 0) {
        LogError << "Got invalid response from PlayTools";
        close();
        return false;
    }

    if (!check_version() || !fetch_screen_size(screen_size_.first, screen_size_.second)) {
        close();
        return false;
    }

    return true;
}

bool PlayToolsClient::check_version()
{
    uint32_t version = 0;
    constexpr char request[6] = { 0, 4, 'V', 'E', 'R', 'N' };

    boost::system::error_code ec;
    boost::asio::write(socket_, boost::asio::buffer(request), ec);
    if (ec) {
        LogError << "Cannot send version request:" << ec.message();
        return false;
    }
    boost::asio::read(socket_, boost::asio::buffer(&version, sizeof(version)), ec);
    if (ec) {
        LogError << "Cannot read version response:" << ec.message();
        return false;
    }

    version = ntohl(version);
    if (version < MinimalVersion) {
        LogError << "Unsupported PlayTools version:" << version << "minimal required:" << MinimalVersion;
        return false;
    }

    LogInfo << "PlayTools version:" << version;
    return true;
}

bool PlayToolsClient::fetch_screen_size(int& width, int& height)
{
    uint16_t w = 0, h = 0;
    constexpr char request[6] = { 0, 4, 'S', 'I', 'Z', 'E' };

    boost::system::error_code ec;
    boost::asio::write(socket_, boost::asio::buffer(request), ec);
    if (ec) {
        LogError << "Cannot send size request:" << ec.message();
        return false;
    }
    boost::asio::read(socket_, boost::asio::buffer(&w, sizeof(w)), ec);
    if (ec) {
        LogError << "Cannot read screen width:" << ec.message();
        return false;
    }
    boost::asio::read(socket_, boost::asio::buffer(&h, sizeof(h)), ec);
    if (ec) {
        LogError << "Cannot read screen height:" << ec.message();
        return false;
    }

    width = ntohs(w);
    height = ntohs(h);

    LogInfo << "Screen size:" << width << "x" << height;
    return true;
}

bool PlayToolsClient::screencap(std::vector<uint8_t>& buffer, int& width, int& height)
{
    if (!open()) {
        return false;
    }

    uint32_t image_size = 0;
    constexpr char request[6] = { 0, 4, 'S', 'C', 'R', 'N' };

    boost::system::error_code ec;
    boost::asio::write(socket_, boost::asio::buffer(request), ec);
    if (ec) {
        LogError << "Cannot send screencap request:" << ec.message();
        close();
        return false;
    }
    boost::asio::read(socket_, boost::asio::buffer(&image_size, sizeof(image_size)), ec);
    if (ec) {
        LogError << "Cannot read screencap size:" << ec.message();
        close();
        return false;
    }
    image_size = ntohl(image_size);

    if (image_size == 0) {
        LogError << "Cannot get screencap: invalid image size";
        return false;
    }

    size_t expected_size = static_cast<size_t>(screen_size_.first) * screen_size_.second * 4;
    if (image_size != expected_size) {
        LogWarn << "Image size mismatch: got" << image_size << "expected" << expected_size;
    }

    buffer.resize(image_size);
    boost::asio::read(socket_, boost::asio::buffer(buffer.data(), image_size), ec);
    if (ec) {
        LogError << "Cannot read screencap data:" << ec.message();
        close();
        return false;
    }

    width = screen_size_.first;
    height = screen_size_.second;
    return true;
}

bool PlayToolsClient::touch(TouchPhase phase, int x, int y)
{
    if (!open()) {
        return false;
    }

    uint16_t nx = htons(static_cast<uint16_t>(x));
    uint16_t ny = htons(static_cast<uint16_t>(y));
    uint8_t payload[5] = { static_cast<uint8_t>(phase), 0, 0, 0, 0 };
    std::memcpy(payload + 1, &nx, sizeof(nx));
    std::memcpy(payload + 3, &ny, sizeof(ny));

    constexpr char request[6] = { 0, 9, 'T', 'U', 'C', 'H' };
    boost::system::error_code ec;
    boost::asio::write(socket_, boost::asio::buffer(request), ec);
    if (ec) {
        LogError << "Cannot send touch request:" << ec.message();
        close();
        return false;
    }
    boost::asio::write(socket_, boost::asio::buffer(payload, 5), ec);
    if (ec) {
        LogError << "Cannot send touch payload:" << ec.message();
        close();
        return false;
    }

    return true;
}

bool PlayToolsClient::terminate()
{
    if (!socket_.is_open()) {
        return true;
    }

    constexpr char request[6] = { 0, 4, 'T', 'E', 'R', 'M' };
    boost::system::error_code ec;
    boost::asio::write(socket_, boost::asio::buffer(request), ec);
    if (ec) {
        LogError << "Cannot terminate game:" << ec.message();
        return false;
    }

    return true;
}

MAA_CTRL_UNIT_NS_END
