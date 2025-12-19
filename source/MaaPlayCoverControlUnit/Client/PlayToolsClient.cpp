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
    std::lock_guard<std::mutex> lock(mutex_);

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

    try {
        socket_.shutdown(tcp::socket::shutdown_both);
    }
    catch (const std::exception& e) {
        LogWarn << "Error during socket shutdown:" << e.what();
    }

    try {
        socket_.close();
    }
    catch (const std::exception& e) {
        LogWarn << "Error during socket close:" << e.what();
    }
}

bool PlayToolsClient::is_connected() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return socket_.is_open() && screen_size_.first > 0;
}

std::pair<int, int> PlayToolsClient::screen_size() const
{
    std::lock_guard<std::mutex> lock(mutex_);
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

    try {
        boost::asio::connect(socket_, resolver.resolve(host, port));
        boost::asio::write(socket_, boost::asio::buffer(handshake));
        boost::asio::read(socket_, boost::asio::buffer(buffer, 4));
    }
    catch (const std::exception& e) {
        LogError << "Cannot connect to" << address_ << e.what();
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

    try {
        boost::asio::write(socket_, boost::asio::buffer(request));
        boost::asio::read(socket_, boost::asio::buffer(&version, sizeof(version)));
    }
    catch (const std::exception& e) {
        LogError << "Cannot get PlayTools version:" << e.what();
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

    try {
        boost::asio::write(socket_, boost::asio::buffer(request));
        boost::asio::read(socket_, boost::asio::buffer(&w, sizeof(w)));
        boost::asio::read(socket_, boost::asio::buffer(&h, sizeof(h)));
    }
    catch (const std::exception& e) {
        LogError << "Cannot get screen resolution:" << e.what();
        return false;
    }

    width = ntohs(w);
    height = ntohs(h);

    LogInfo << "Screen size:" << width << "x" << height;
    return true;
}

bool PlayToolsClient::screencap(std::vector<uint8_t>& buffer, int& width, int& height)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!open()) {
        return false;
    }

    uint32_t image_size = 0;
    constexpr char request[6] = { 0, 4, 'S', 'C', 'R', 'N' };

    try {
        boost::asio::write(socket_, boost::asio::buffer(request));
        boost::asio::read(socket_, boost::asio::buffer(&image_size, sizeof(image_size)));
        image_size = ntohl(image_size);
    }
    catch (const std::exception& e) {
        LogError << "Cannot get screencap:" << e.what();
        close();
        return false;
    }

    if (image_size == 0) {
        LogError << "Cannot get screencap: invalid image size";
        return false;
    }

    size_t expected_size = static_cast<size_t>(screen_size_.first) * screen_size_.second * 4;
    if (image_size != expected_size) {
        LogWarn << "Image size mismatch: got" << image_size << "expected" << expected_size;
    }

    try {
        buffer.resize(image_size);
        boost::asio::read(socket_, boost::asio::buffer(buffer.data(), image_size));
    }
    catch (const std::exception& e) {
        LogError << "Cannot read screencap data:" << e.what();
        close();
        return false;
    }

    width = screen_size_.first;
    height = screen_size_.second;
    return true;
}

bool PlayToolsClient::touch(TouchPhase phase, int x, int y)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!open()) {
        return false;
    }

    uint16_t nx = htons(static_cast<uint16_t>(x));
    uint16_t ny = htons(static_cast<uint16_t>(y));
    uint8_t payload[5] = { static_cast<uint8_t>(phase), 0, 0, 0, 0 };
    std::memcpy(payload + 1, &nx, sizeof(nx));
    std::memcpy(payload + 3, &ny, sizeof(ny));

    try {
        constexpr char request[6] = { 0, 9, 'T', 'U', 'C', 'H' };
        boost::asio::write(socket_, boost::asio::buffer(request));
        boost::asio::write(socket_, boost::asio::buffer(payload, 5));
    }
    catch (const std::exception& e) {
        LogError << "Cannot touch screen:" << e.what();
        close();
        return false;
    }

    return true;
}

bool PlayToolsClient::terminate()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!socket_.is_open()) {
        return true;
    }

    try {
        constexpr char request[6] = { 0, 4, 'T', 'E', 'R', 'M' };
        boost::asio::write(socket_, boost::asio::buffer(request));
    }
    catch (const std::exception& e) {
        LogError << "Cannot terminate game:" << e.what();
        return false;
    }

    return true;
}

MAA_CTRL_UNIT_NS_END
