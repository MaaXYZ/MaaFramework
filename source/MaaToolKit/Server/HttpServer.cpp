#include "HttpServer.h"
#include "ApiDispatcher.h"
#include "Framework/Init.h"
#include "RequestResponse.h"

#include <meojson/json.hpp>

using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;

MAA_TOOLKIT_SERVER_BEGIN

inline void write_ws(websocket::stream<tcp::socket>& ws, const std::string& str)
{
    ws.write(boost::asio::buffer(str.c_str(), str.size() + 1));
}

static json::object handle_request_ws(websocket::stream<tcp::socket>& ws, boost::beast::flat_buffer& buffer)
{
    if (!ws.got_text()) {
        return { { "success", false }, { "error", "binary not supported" } };
    }

    auto req = json::parse(boost::beast::buffers_to_string(buffer.data()));
    if (!req || !req->is_object()) {
        return { { "success", false }, { "error", "json parse failed" } };
    }

    auto res = SingletonHolder<ApiDispatcher>::get_instance().handle_route(req->as_object());
    if (res.has_value()) {
        return { { "success", true }, { "data", res.value() } };
    }
    else {
        return { { "success", false }, { "error", "internal error" } };
    }
}

static void handle_session_ws(tcp::socket& socket, boost::beast::http::request<boost::beast::http::string_body>& req)
{
    using namespace boost::beast;

    websocket::stream<tcp::socket> ws(std::move(socket));
    error_code ec;

    ws.accept(req, ec);
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

        write_ws(ws, handle_request_ws(ws, buffer).to_string());
    }
}

static boost::beast::http::message_generator handle_request(
    boost::beast::http::request<boost::beast::http::string_body>&& request)
{
    RequestResponse rr(std::move(request));

    if (rr.get_request().method() != boost::beast::http::verb::post) {
        rr.reply_bad_request("only post supported");
    }
    else {
        auto req = rr.request_body_json();
        if (req.has_value()) {
            SingletonHolder<ApiDispatcher>::get_instance().handle_route(req.value(), [](const json::object&) {});
            if (!rr.has_response()) {
                rr.reply_error("internal error", boost::beast::http::status::internal_server_error);
            }
        }
        else {
            rr.reply_bad_request("json parse error");
        }
    }

    return rr.take_response();
}

static void handle_session(tcp::socket& socket)
{
    using namespace boost::beast;

    error_code ec;
    flat_buffer buf;

    while (true) {
        http::request<http::string_body> request;
        http::read(socket, buf, request, ec);
        if (ec == http::error::end_of_stream) {
            break;
        }
        if (ec) {
            // ERROR
            break;
        }
        if (websocket::is_upgrade(request)) {
            handle_session_ws(socket, request);
            return;
        }
        http::message_generator msg = handle_request(std::move(request));
        auto keep_alive = msg.keep_alive();
        write(socket, std::move(msg), ec);
        if (ec) {
            // ERROR
            break;
        }
        if (!keep_alive) {
            break;
        }
    }

    socket.shutdown(tcp::socket::shutdown_send, ec);
}

bool HttpServer::start(std::string_view ip, uint16_t port)
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

bool HttpServer::stop()
{
    stopping = true;
    acceptor->close();
    dispatcher->join();
    acceptor = nullptr;
    dispatcher = nullptr;
    return true;
}

MAA_TOOLKIT_SERVER_END
