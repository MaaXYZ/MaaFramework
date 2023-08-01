#include "WsServer.h"
#include "ApiDispatcher.h"
#include "Framework/Init.h"
#include "RequestResponse.h"

#include <meojson/json.hpp>

using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;

MAA_TOOLKIT_NS_BEGIN

inline void write_ws(websocket::stream<tcp::socket>& ws, const std::string& str)
{
    ws.write(boost::asio::buffer(str.c_str(), str.size() + 1));
}

static void handle_session(tcp::socket& socket)
{
    using namespace boost::beast;

    websocket::stream<tcp::socket> ws(std::move(socket));
    error_code ec;

    ws.accept(ec);
    if (ec) {
        return;
    }

    while (true) {
        flat_buffer buffer;

        // Read a message
        ws.read(buffer, ec);
        if (ec == http::error::end_of_stream) {
            break;
        }
        if (ec) {
            // ERROR
            break;
        }

        if (!ws.got_text()) {
            write_ws(ws, R"({ "success": false, "error": "binary format not supported" })");
            continue;
        }

        auto req = json::parse(buffers_to_string(buffer.data()));
        if (!req || !req->is_object()) {
            write_ws(ws, R"({ "success": false, "error": "json parse failed" })");
            continue;
        }

        auto res = SingletonHolder<ApiDispatcher>::get_instance().handle_route(req->as_object());
        if (res.has_value()) {
            write_ws(ws, (json::object { { "success", true }, { "data", res.value() } }).to_string());
        }
        else {
            write_ws(ws, R"({ "success": false, "error": "internal error" })");
        }
    }
}

bool WsServer::start(std::string_view ip, uint16_t port)
{
    if (acceptor) {
        return false;
    }

    SingletonHolder<ApiDispatcher>::get_instance().init();

    auto address = boost::asio::ip::make_address(ip);

    // TODO: 不知道为啥make_shared直接传参匹配不到
    tcp::acceptor acc { ctx, { address, port } };
    acceptor = std::make_shared<tcp::acceptor>(std::move(acc));

    stopping = false;
    dispatcher = std::make_shared<std::thread>([this]() {
        boost::system::error_code ec;
        while (!stopping) {
            tcp::socket socket(ctx);
            acceptor->accept(socket, ec);
            if (ec) {
                break;
            }
            std::thread([](tcp::socket&& sock) { handle_session(sock); }, std::move(socket)).detach();
        }
    });

    return true;
}

bool WsServer::stop()
{
    stopping = true;
    acceptor->close();
    dispatcher->join();
    acceptor = nullptr;
    dispatcher = nullptr;
    return true;
}

MAA_TOOLKIT_NS_END
