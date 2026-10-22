#pragma once
#include <memory>
#include <meojson/json.hpp>
#include <string>
#include <vector>
#include <xdb/xdb.h>

namespace maa::ios_unit
{
class XdbClient
{
public:
    ~XdbClient() { close(); }

    XdbClient() = default;
    XdbClient(const XdbClient&) = delete;
    XdbClient& operator=(const XdbClient&) = delete;

    bool connect(const std::string& config, std::string& error)
    {
        auto value = json::parse(config.empty() ? "{}" : config);
        if (!value || !value->is_object()) {
            error = "Invalid xdb server configuration";
            return false;
        }
        auto port = value->get("server_port", 0);
        auto token = value->get("server_token", std::string());
        if (port < 0 || port > 65535) {
            error = "Invalid xdb server port";
            return false;
        }
        close();
        client_ = xdb_client_connect(static_cast<uint16_t>(port), token.c_str(), nullptr, nullptr);
        if (!client_) {
            error = xdb_last_error();
        }
        return client_ != nullptr;
    }

    bool call(const char* method, const json::value& params, json::value& result, std::vector<uint8_t>& data, std::string& error)
    {
        xdb_response* raw = nullptr;
        if (!xdb_client_request(client_, method, params.to_string().c_str(), &raw)) {
            error = xdb_last_error();
            return false;
        }
        std::unique_ptr<xdb_response, decltype(&xdb_response_destroy)> response(raw, xdb_response_destroy);
        auto envelope = json::parse(xdb_response_json(raw));
        if (!envelope || !envelope->get("ok", false)) {
            error = envelope ? envelope->get("error", "message", std::string("xdb request failed")) : "Invalid xdb response";
            return false;
        }
        result = envelope->get("result", json::value(json::object { }));
        data.clear();
        if (auto size = xdb_response_size(raw)) {
            data.assign(xdb_response_data(raw), xdb_response_data(raw) + size);
        }
        error.clear();
        return true;
    }

    bool call(const char* method, const json::value& params, std::string& error)
    {
        json::value result;
        std::vector<uint8_t> data;
        return call(method, params, result, data, error);
    }

    void interrupt() { xdb_client_interrupt(client_); }

    void close()
    {
        xdb_client_destroy(client_);
        client_ = nullptr;
    }

    bool connected() const { return client_ != nullptr; }

private:
    xdb_client* client_ = nullptr;
};
}
