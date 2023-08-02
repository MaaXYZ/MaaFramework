#include "RequestResponse.h"
#include "JsonValidator.hpp"

MAA_TOOLKIT_SERVER_BEGIN

std::optional<json::object> RequestResponse::request_body_json()
{
    auto body = request.body();
    auto res = json::parse(body);
    if (!res || !res->is_object()) {
        return std::nullopt;
    }
    return res->as_object();
}

void RequestResponse::reply_json(json::object object, boost::beast::http::status status)
{
    using namespace boost::beast;

    if (response) {
        return;
    }

    http::response<http::string_body> res(status, request.version());
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "application/json");
    auto json = object.to_string();
    res.content_length(json.size());
    res.keep_alive(request.keep_alive());
    res.body() = json;
    res.prepare_payload();
    response = std::make_shared<http::message_generator>(std::move(res));
}

void RequestResponse::reply_file(void* data, uint64_t size, std::string_view mime, boost::beast::http::status status)
{
    using namespace boost::beast;

    if (response) {
        return;
    }

    http::response<http::buffer_body> res(status, request.version());
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime);
    res.content_length(size);
    res.keep_alive(request.keep_alive());
    http::buffer_body::value_type buf;
    buf.data = data;
    buf.size = size;
    buf.more = false;
    res.body() = buf;
    res.prepare_payload();
    response = std::make_shared<http::message_generator>(std::move(res));
}

MAA_TOOLKIT_SERVER_END
