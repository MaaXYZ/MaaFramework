#pragma once

#include "Utils/Boost.hpp"

#include <meojson/json.hpp>

MAA_TOOLKIT_SERVER_BEGIN

class RequestResponse
{
public:
    RequestResponse(boost::beast::http::request<boost::beast::http::string_body>&& req) : request(req) {}

    std::optional<json::object> request_body_json();

    void reply_json(json::object object, boost::beast::http::status status = boost::beast::http::status::ok);
    void reply_file(void* data, uint64_t size, std::string_view mime,
                    boost::beast::http::status status = boost::beast::http::status::ok);

    void reply_ok(json::object object) { reply_json({ { "success", true }, { "data", object } }); }
    void reply_error(const char* why, boost::beast::http::status status)
    {
        reply_json({ { "success", false }, { "error", why } }, status);
    }

    void reply_bad_request(const char* why = "bad request")
    {
        reply_error(why, boost::beast::http::status::bad_request);
    }

    const boost::beast::http::request<boost::beast::http::string_body>& get_request() { return request; }

    bool has_response() { return response.get(); }

    boost::beast::http::message_generator take_response()
    {
        boost::beast::http::message_generator res = std::move(*response);
        response = nullptr;
        return res;
    }

private:
    boost::beast::http::request<boost::beast::http::string_body> request;
    std::shared_ptr<boost::beast::http::message_generator> response;
};

MAA_TOOLKIT_SERVER_END
