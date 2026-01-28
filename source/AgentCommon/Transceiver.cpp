#include "MaaAgent/Transceiver.h"

#include <format>
#include <fstream>
#include <optional>

#ifdef _WIN32
#include "MaaUtils/SafeWindows.hpp"
// afunix.h 在旧版 SDK 中可能不存在，直接定义 AF_UNIX 常量
#ifndef AF_UNIX
#define AF_UNIX 1
#endif
#endif

#include "MaaUtils/Platform.h"
#include "MaaUtils/StringMisc.hpp"
#include "MaaUtils/Uuid.h"

MAA_AGENT_NS_BEGIN

Transceiver::~Transceiver()
{
    LogFunc;

    uninit_socket();
}

bool Transceiver::handle_image_header(const json::value& j)
{
    if (!j.is<ImageHeader>()) {
        return false;
    }

    const ImageHeader& header = j.as<ImageHeader>();

    LogTrace << VAR(header) << VAR(ipc_addr_);

    handle_image(header);

    return true;
}

bool Transceiver::handle_image_encoded_header(const json::value& j)
{
    if (!j.is<ImageEncodedHeader>()) {
        return false;
    }

    const ImageEncodedHeader& header = j.as<ImageEncodedHeader>();

    LogTrace << VAR(header) << VAR(ipc_addr_);

    handle_image_encoded(header);

    return true;
}

static std::string temp_directory()
{
#ifndef _WIN32
    auto path = std::filesystem::temp_directory_path();
#else
    // https://github.com/MaaEnd/MaaEnd/issues/95
    // 测了写权限了还不行，我没招了，直接拉💩
    auto path = MaaNS::path("C:/Temp");

    if (!std::filesystem::exists(path)) {
        std::error_code ec;
        std::filesystem::create_directories(path, ec);
    }
#endif

    return path_to_utf8_string(path);
}

void Transceiver::init_socket(const std::string& identifier, bool bind)
{
    LogFunc << VAR(bind);

    static auto kTempDir = temp_directory();

    std::string path = std::format("{}/maafw-agent-{}.sock", kTempDir, identifier);
    ipc_addr_ = std::format("ipc://{}", path);
    ipc_path_ = MaaNS::path(path);
    is_tcp_ = false;

    LogInfo << VAR(ipc_addr_) << VAR(identifier);

    zmq_sock_ = zmq::socket_t(zmq_ctx_, zmq::socket_type::pair);

    zmq_pollitem_send_ = zmq::pollitem_t(zmq_sock_.handle(), 0, ZMQ_POLLOUT, 0);
    zmq_pollitem_recv_ = zmq::pollitem_t(zmq_sock_.handle(), 0, ZMQ_POLLIN, 0);

    is_bound_ = bind;

    if (is_bound_) {
        zmq_sock_.bind(ipc_addr_);
    }
    else {
        zmq_sock_.connect(ipc_addr_);
    }
}

static uint16_t parse_port_from_endpoint(const std::string& endpoint)
{
    // endpoint 格式为 "tcp://127.0.0.1:12345"
    auto pos = endpoint.rfind(':');
    if (pos == std::string::npos || pos >= endpoint.size() - 1) {
        return 0;
    }
    std::string port_str = endpoint.substr(pos + 1);
    
    // 验证字符串是否为有效数字
    if (port_str.empty() || !std::all_of(port_str.begin(), port_str.end(), [](unsigned char c) {
        return std::isdigit(c) != 0;
    })) {
        return 0;
    }
    
    // 使用 strtoul 替代 stoi，避免异常并处理溢出
    char* end = nullptr;
    unsigned long port = std::strtoul(port_str.c_str(), &end, 10);
    if (end != port_str.c_str() + port_str.size() || port > 65535) {
        return 0;
    }
    
    return static_cast<uint16_t>(port);
}

uint16_t Transceiver::init_tcp_socket(uint16_t port, bool bind)
{
    LogFunc << VAR(port) << VAR(bind);

    is_tcp_ = true;

    zmq_sock_ = zmq::socket_t(zmq_ctx_, zmq::socket_type::pair);

    zmq_pollitem_send_ = zmq::pollitem_t(zmq_sock_.handle(), 0, ZMQ_POLLOUT, 0);
    zmq_pollitem_recv_ = zmq::pollitem_t(zmq_sock_.handle(), 0, ZMQ_POLLIN, 0);

    is_bound_ = bind;

    if (is_bound_) {
        // 如果 port 为 0，使用通配符让系统自动分配端口
        std::string bind_addr = std::format("tcp://127.0.0.1:{}", port == 0 ? "*" : std::to_string(port));
        zmq_sock_.bind(bind_addr);

        // 获取实际绑定的端点
        char endpoint[256] = {};
        size_t endpoint_len = sizeof(endpoint);
        zmq_getsockopt(zmq_sock_.handle(), ZMQ_LAST_ENDPOINT, endpoint, &endpoint_len);
        ipc_addr_ = endpoint;

        tcp_port_ = parse_port_from_endpoint(ipc_addr_);
        LogInfo << "TCP socket bound" << VAR(ipc_addr_) << VAR(tcp_port_);
    }
    else {
        ipc_addr_ = std::format("tcp://127.0.0.1:{}", port);
        tcp_port_ = port;
        zmq_sock_.connect(ipc_addr_);
        LogInfo << "TCP socket connected" << VAR(ipc_addr_);
    }

    return tcp_port_;
}

bool Transceiver::should_fallback_to_tcp()
{
#ifdef _WIN32
    // Windows 从 Build 17063 开始支持 AF_UNIX (Unix Domain Socket)
    // 直接尝试创建 socket 来检测系统是否支持，比检测版本号更可靠

    static std::optional<bool> cached_result;
    if (cached_result.has_value()) {
        return *cached_result;
    }

    // 需要先初始化 Winsock，否则 socket() 会失败
    WSADATA wsa_data;
    int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (wsa_result != 0) {
        LogWarn << "WSAStartup failed, falling back to TCP" << VAR(wsa_result);
        cached_result = true;
        return true;
    }

    SOCKET test_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (test_sock == INVALID_SOCKET) {
        int err = WSAGetLastError();
        WSACleanup();
        // WSAEAFNOSUPPORT (10047) 表示地址族不支持
        LogWarn << "AF_UNIX socket not supported on this Windows version, falling back to TCP" << VAR(err);
        cached_result = true;
        return true;
    }

    closesocket(test_sock);
    WSACleanup();
    cached_result = false;
    return false;
#else
    // 非 Windows 系统通常 IPC 工作正常
    return false;
#endif
}

void Transceiver::uninit_socket()
{
    LogFunc << VAR(ipc_addr_);

    // if (connected()) {
    //     if (is_bound_) {
    //         zmq_sock_.unbind(ipc_addr_);
    //     }
    //     else {
    //         zmq_sock_.disconnect(ipc_addr_);
    //     }
    // }

    zmq_sock_.close();
    zmq_ctx_.close();

    // TCP 模式无需删除 socket 文件
    if (!is_tcp_) {
        std::error_code ec;
        std::filesystem::remove(ipc_path_, ec);
    }
}

bool Transceiver::alive()
{
    std::unique_lock lock(socket_mutex_);
    return zmq_sock_.handle() != nullptr && zmq::detail::poll(&zmq_pollitem_send_, 1, 0);
}

void Transceiver::set_timeout(const std::chrono::milliseconds& timeout)
{
    LogInfo << VAR(timeout) << VAR(ipc_addr_);
    timeout_ = timeout;
}

bool Transceiver::poll(zmq::pollitem_t& pollitem)
{
    const auto start_clock = std::chrono::steady_clock::now();

    while (true) {
        auto elapsed = duration_since(start_clock);
        auto remaining_time = timeout_ > elapsed ? timeout_ - elapsed : std::chrono::milliseconds(0);
        auto interval = std::min(remaining_time, std::chrono::milliseconds(1000));

        if (zmq::poll(&pollitem, 1, interval)) {
            return true;
        }

        if (elapsed > timeout_) {
            LogWarn << "socket is not alive" << VAR(elapsed) << VAR(timeout_) << VAR(ipc_addr_);
            return false;
        }
    }
    // unreachable code
    // return false;
}

bool Transceiver::send(const json::value& j)
{
    // LogTrace << VAR(j.to_string()) << VAR(ipc_addr_);

    std::unique_lock lock(socket_mutex_);

    if (!poll(zmq_pollitem_send_)) {
        LogError << "send canceled";
        return false;
    }

    std::string jstr = j.dumps();
    zmq::message_t msg(jstr.data(), jstr.size());

    bool sent = zmq_sock_.send(std::move(msg), zmq::send_flags::dontwait).has_value();
    if (!sent) {
        LogError << "failed to send msg" << VAR(j);
        return false;
    }
    return true;
}

std::optional<json::value> Transceiver::recv()
{
    std::unique_lock lock(socket_mutex_);

    if (!poll(zmq_pollitem_recv_)) {
        LogError << "recv canceled";
        return std::nullopt;
    }

    zmq::message_t msg;

    auto size_opt = zmq_sock_.recv(msg, zmq::recv_flags::dontwait);
    if (!size_opt || *size_opt == 0) {
        LogError << "failed to recv msg" << VAR(ipc_addr_);
        return std::nullopt;
    }

    std::string_view init_str = msg.to_string_view();
    auto jopt = json::parse(init_str);
    if (!jopt) {
        LogError << "failed to parse msg" << VAR(ipc_addr_);
        return std::nullopt;
    }
    auto j = *std::move(jopt);
    // LogTrace << VAR(j.to_string()) << VAR(ipc_addr_);

    return j;
}

std::string Transceiver::send_image(const cv::Mat& mat)
{
    if (mat.empty()) {
        LogWarn << "empty image" << VAR(ipc_addr_);
        return {};
    }

    std::unique_lock lock(socket_mutex_);

    ImageHeader header {
        .uuid = make_uuid(),
        .rows = mat.rows,
        .cols = mat.cols,
        .type = mat.type(),
        .size = mat.total() * mat.elemSize(),
    };

    // send header
    if (!poll(zmq_pollitem_send_)) {
        LogError << "send header canceled";
        return {};
    }
    std::string jstr = json::value(header).dumps();
    zmq::message_t header_msg(jstr.data(), jstr.size());
    bool sent = zmq_sock_.send(std::move(header_msg), zmq::send_flags::dontwait).has_value();
    if (!sent) {
        LogError << "failed to send header" << VAR(header) << VAR(ipc_addr_);
        return {};
    }

    // send image data
    zmq::message_t img_msg(mat.data, mat.total() * mat.elemSize());
    sent = zmq_sock_.send(img_msg, zmq::send_flags::none).has_value();
    if (!sent) {
        LogError << "failed to send msg" << VAR(ipc_addr_);
        return {};
    }
    return header.uuid;
}

std::string Transceiver::send_image_encoded(const ImageEncodedBuffer& encoded_data)
{
    if (encoded_data.empty()) {
        LogWarn << "empty encoded data" << VAR(ipc_addr_);
        return {};
    }

    std::unique_lock lock(socket_mutex_);

    ImageEncodedHeader header {
        .uuid = make_uuid(),
        .size = encoded_data.size(),
    };

    // send header
    if (!poll(zmq_pollitem_send_)) {
        LogError << "send encoded header canceled";
        return {};
    }
    std::string jstr = json::value(header).dumps();
    zmq::message_t header_msg(jstr.data(), jstr.size());
    bool sent = zmq_sock_.send(std::move(header_msg), zmq::send_flags::dontwait).has_value();
    if (!sent) {
        LogError << "failed to send encoded header" << VAR(header) << VAR(ipc_addr_);
        return {};
    }

    // send encoded image data
    zmq::message_t img_msg(encoded_data.data(), encoded_data.size());
    sent = zmq_sock_.send(img_msg, zmq::send_flags::none).has_value();
    if (!sent) {
        LogError << "failed to send encoded image data" << VAR(ipc_addr_);
        return {};
    }
    return header.uuid;
}

cv::Mat Transceiver::get_image_cache(const std::string& uuid)
{
    if (uuid.empty()) {
        LogWarn << "empty uuid" << VAR(ipc_addr_);
        return {};
    }

    auto it = recved_images_.find(uuid);
    if (it == recved_images_.end()) {
        LogError << "image not found" << VAR(uuid) << VAR(ipc_addr_);
        return {};
    }

    cv::Mat image = std::move(it->second);
    recved_images_.erase(it);

    return image;
}

Transceiver::ImageEncodedBuffer Transceiver::get_image_encoded_cache(const std::string& uuid)
{
    if (uuid.empty()) {
        LogWarn << "empty uuid" << VAR(ipc_addr_);
        return {};
    }

    auto it = recved_images_encoded_.find(uuid);
    if (it == recved_images_encoded_.end()) {
        LogError << "encoded image not found" << VAR(uuid) << VAR(ipc_addr_);
        return {};
    }

    ImageEncodedBuffer encoded_data = std::move(it->second);
    recved_images_encoded_.erase(it);

    return encoded_data;
}

void Transceiver::handle_image(const ImageHeader& header)
{
    LogFunc << VAR(header);

    std::unique_lock lock(socket_mutex_);

    zmq::message_t msg;
    auto size_opt = zmq_sock_.recv(msg);
    if (!size_opt || *size_opt == 0) {
        LogError << "failed to recv msg" << VAR(ipc_addr_);
        return;
    }

    if (header.size != msg.size()) {
        LogError << "size mismatch" << VAR(header.size) << VAR(msg.size());
        return;
    }

    cv::Mat image = cv::Mat(header.rows, header.cols, header.type, msg.data()).clone();
    recved_images_.insert_or_assign(header.uuid, std::move(image));
}

void Transceiver::handle_image_encoded(const ImageEncodedHeader& header)
{
    LogFunc << VAR(header);

    std::unique_lock lock(socket_mutex_);

    zmq::message_t msg;
    auto size_opt = zmq_sock_.recv(msg);
    if (!size_opt || *size_opt == 0) {
        LogError << "failed to recv encoded image data" << VAR(ipc_addr_);
        return;
    }

    if (header.size != msg.size()) {
        LogError << "encoded size mismatch" << VAR(header.size) << VAR(msg.size());
        return;
    }

    ImageEncodedBuffer encoded_data(static_cast<const uint8_t*>(msg.data()), static_cast<const uint8_t*>(msg.data()) + msg.size());
    recved_images_encoded_.insert_or_assign(header.uuid, std::move(encoded_data));
}

MAA_AGENT_NS_END
