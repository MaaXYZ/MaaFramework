#include "MaaAgent/Transceiver.h"

#include <format>

#ifdef _WIN32
#include "MaaUtils/SafeWindows.hpp"
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
    auto path = std::filesystem::temp_directory_path();

#ifdef _WIN32

    bool fallback = false;

    // ZeroMQ IPC 在 Windows 上不支持 Unicode 路径，拉💩
    if (GetACP() != CP_UTF8 && std::ranges::any_of(path.native(), [](wchar_t ch) { return ch > 127; })) {
        fallback = true;
    }
    else if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
        fallback = true;
    }

    if (fallback) {
        path = MaaNS::path("C:/Temp");
        if (!std::filesystem::exists(path)) {
            std::filesystem::create_directories(path);
        }
    }

#endif

    return path_to_utf8_string(path);
}

void Transceiver::init_socket(const std::string& identifier, bool bind)
{
    static auto kTempDir = temp_directory();

    std::string path = std::format("{}/maafw-agent-{}.sock", kTempDir, identifier);
    ipc_addr_ = std::format("ipc://{}", path);
    ipc_path_ = MaaNS::path(path);

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

    std::error_code ec;
    std::filesystem::remove(ipc_path_, ec);
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
