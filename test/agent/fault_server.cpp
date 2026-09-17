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
