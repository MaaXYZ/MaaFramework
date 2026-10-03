#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <utility>

#include <meojson/json.hpp>
#include <zmq.hpp>

#include "MaaAgent/Message.hpp"

namespace
{

constexpr int kReceiveTimeoutMs = 5000;

bool receive(zmq::socket_t& socket, json::value& message)
{
    zmq::message_t raw;
    if (!socket.recv(raw, zmq::recv_flags::none)) {
        return false;
    }

    auto parsed = json::parse(raw.to_string_view());
    if (!parsed) {
        return false;
    }

    message = std::move(*parsed);
    return true;
}

bool send(zmq::socket_t& socket, const json::value& message)
{
    auto serialized = message.dumps();
    return socket.send(zmq::buffer(serialized), zmq::send_flags::none).has_value();
}

constexpr const char* kReqIdKey = "_req_id";
constexpr const char* kRespIdKey = "_resp_id";
constexpr int64_t kPoisonCount = 64;
// 与 agent_tcp_main_test.py 中的 FAULT_REC_BOX 保持一致
constexpr std::array<int32_t, 4> kFaultRecBox { 11, 12, 13, 14 };

bool reply(zmq::socket_t& socket, json::value response, const json::value& request)
{
    if (auto req_id = request.find<int64_t>(kReqIdKey)) {
        response[kRespIdKey] = *req_id;
    }
    return send(socket, response);
}

// 先为 client 之后才会发出的请求编号预发一批带毒回包，再正常应答。
// client 只应认领仍在等待的编号：这些回包必须被丢弃，不能被之后恰好拿到该编号的请求认领
int serve_inactive_response_id(zmq::socket_t& socket, const json::value& startup)
{
    auto startup_id = startup.find<int64_t>(kReqIdKey);
    if (!startup_id) {
        return 7;
    }

    const MAA_AGENT_NS::CustomRecognitionResponse poison { .ret = true, .out_box = { 9, 9, 9, 9 }, .out_detail = "poison" };
    for (int64_t offset = 1; offset <= kPoisonCount; ++offset) {
        json::value message = poison;
        message[kRespIdKey] = *startup_id + offset;
        if (!send(socket, message)) {
            return 4;
        }
    }

    MAA_AGENT_NS::StartUpResponse response;
    response.recognitions.emplace_back("FaultRec");
    if (!reply(socket, response, startup)) {
        return 4;
    }

    while (true) {
        zmq::message_t raw;
        if (!socket.recv(raw, zmq::recv_flags::none)) {
            return 6;
        }
        auto parsed = json::parse(raw.to_string_view());
        if (!parsed) {
            // 识别请求前发来的图像数据
            continue;
        }
        const json::value& message = *parsed;

        if (message.is<MAA_AGENT_NS::ContextEventRequest>()) {
            if (!reply(socket, MAA_AGENT_NS::ContextEventResponse { }, message)) {
                return 4;
            }
        }
        else if (message.is<MAA_AGENT_NS::CustomRecognitionRequest>()) {
            const MAA_AGENT_NS::CustomRecognitionResponse fault_rec { .ret = true, .out_box = kFaultRecBox, .out_detail = "fault" };
            if (!reply(socket, fault_rec, message)) {
                return 4;
            }
        }
        else if (message.is<MAA_AGENT_NS::ShutDownRequest>()) {
            return reply(socket, MAA_AGENT_NS::ShutDownResponse { }, message) ? 0 : 4;
        }
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 3) {
        return 1;
    }

    const std::string mode = argv[1];
    const std::string endpoint = std::string("tcp://127.0.0.1:") + argv[2];

    zmq::context_t context;
    zmq::socket_t socket(context, zmq::socket_type::pair);
    socket.set(zmq::sockopt::rcvtimeo, kReceiveTimeoutMs);
    socket.connect(endpoint);

    zmq::pollitem_t writable { socket.handle(), 0, ZMQ_POLLOUT, 0 };
    if (!zmq::poll(&writable, 1, std::chrono::milliseconds(kReceiveTimeoutMs))) {
        return 2;
    }
    std::cout << "ready" << std::endl;

    json::value message;
    if (!receive(socket, message) || !message.is<MAA_AGENT_NS::StartUpRequest>()) {
        return 3;
    }

    if (mode == "inactive-response-id") {
        return serve_inactive_response_id(socket, message);
    }

    if (mode == "protocol-mismatch") {
        MAA_AGENT_NS::StartUpResponse response;
        response.protocol = MAA_AGENT_NS::kProtocolVersion - 1;
        if (!send(socket, response)) {
            return 4;
        }
    }
    else if (mode == "registration-conflict") {
        MAA_AGENT_NS::StartUpResponse response;
        response.actions.emplace_back("FaultConflict");
        if (!send(socket, response)) {
            return 4;
        }
    }
    else if (mode != "drop-startup-response") {
        return 5;
    }

    if (!receive(socket, message) || !message.is<MAA_AGENT_NS::ShutDownRequest>()) {
        return 6;
    }

    return 0;
}
