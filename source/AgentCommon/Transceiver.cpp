#include "MaaAgent/Transceiver.h"

#include <format>

#include "Utils/Platform.h"
#include "Utils/Uuid.h"

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

void Transceiver::init_socket(const std::string& identifier, bool bind)
{
    static auto kTempDir = std::filesystem::temp_directory_path();
    constexpr std::string_view kAddrFormat = "ipc://{}/maafw-agent-{}.sock";

    ipc_addr_ = std::format(kAddrFormat, path_to_utf8_string(kTempDir), identifier);

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
}

bool Transceiver::alive()
{
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
    LogTrace << VAR(j.to_string()) << VAR(ipc_addr_);

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
    LogTrace << VAR(j.to_string()) << VAR(ipc_addr_);

    return j;
}

std::string Transceiver::send_image(const cv::Mat& mat)
{
    if (mat.empty()) {
        LogWarn << "empty image" << VAR(ipc_addr_);
        return {};
    }

    ImageHeader header {
        .uuid = make_uuid(),
        .rows = mat.rows,
        .cols = mat.cols,
        .type = mat.type(),
        .size = mat.total() * mat.elemSize(),
    };

    bool sent = send(header);
    if (!sent) {
        LogError << "failed to send header" << VAR(header) << VAR(ipc_addr_);
        return {};
    }

    zmq::message_t msg(mat.data, mat.total() * mat.elemSize());
    sent = zmq_sock_.send(msg, zmq::send_flags::none).has_value();
    if (!sent) {
        LogError << "failed to send msg" << VAR(ipc_addr_);
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

void Transceiver::handle_image(const ImageHeader& header)
{
    LogFunc << VAR(header);

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

MAA_AGENT_NS_END
