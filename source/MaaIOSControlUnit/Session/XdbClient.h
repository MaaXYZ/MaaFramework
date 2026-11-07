#pragma once
#include <memory>
#include <functional>
#include <atomic>
#include <charconv>
#include <array>
#include <string_view>
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

    bool connect(const std::string& config, std::string& error, std::function<bool()> cancelled = {})
    {
        auto value = json::parse(config.empty() ? "{}" : config);
        if (!value || !value->is_object()) {
            error = "Invalid xdb server configuration";
            return false;
        }
        auto port = value->get("server_port", 0);
        if (port < 0 || port > 65535) {
            error = "Invalid xdb server port";
            return false;
        }
        close();
        const auto cancel = [](void* context) -> int {
            return (*static_cast<std::function<bool()>*>(context))();
        };
        client_ = xdb_client_connect_cancellable(
            static_cast<uint16_t>(port), nullptr, nullptr, cancelled ? +cancel : nullptr, &cancelled);
        if (!client_) {
            error = xdb_last_error();
            return false;
        }
        const char* version = xdb_client_server_version(client_);
        if (!supported_version(version ? version : "")) {
            error = "iOS requires xdb Server 1.6.0 or later; upgrade the running server (reported version: "
                    + std::string(version ? version : "") + ")";
            close();
            return false;
        }
        alive_ = true;
        return true;
    }

    bool call(const char* method, const json::value& params, json::value& result, std::vector<uint8_t>& data, std::string& error)
    {
        xdb_response* raw = nullptr;
        if (!xdb_client_request(client_, method, params.to_string().c_str(), &raw)) {
            error = xdb_last_error();
            alive_ = false;
            return false;
        }
        std::unique_ptr<xdb_response, decltype(&xdb_response_destroy)> response(raw, xdb_response_destroy);
        auto envelope = json::parse(xdb_response_json(raw));
        if (!envelope || !envelope->get("ok", false)) {
            if (!envelope || envelope->get("error", "code", std::string()) == "not_connected") {
                alive_ = false;
            }
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
        alive_ = false;
    }

    bool connected() const { return alive_; }
    bool has_client() const { return client_ != nullptr; }

private:
    static bool supported_version(std::string_view version)
    {
        std::array<unsigned, 3> parts {};
        for (size_t i = 0; i < parts.size(); ++i) {
            const auto parsed = std::from_chars(version.data(), version.data() + version.size(), parts[i]);
            if (parsed.ec != std::errc {} || parsed.ptr == version.data()) {
                return false;
            }
            version.remove_prefix(parsed.ptr - version.data());
            if (i + 1 < parts.size()) {
                if (version.empty() || version.front() != '.') {
                    return false;
                }
                version.remove_prefix(1);
            }
        }
        if (!version.empty() && version.front() != '-' && version.front() != '+') {
            return false;
        }
        constexpr std::array<unsigned, 3> minimum { 1, 6, 0 };
        return parts > minimum || (parts == minimum && !version.starts_with('-'));
    }

    xdb_client* client_ = nullptr;
    std::atomic<bool> alive_ { false };
};
}
