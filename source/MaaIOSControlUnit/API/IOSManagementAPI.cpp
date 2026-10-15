#include "MaaControlUnit/IOSControlUnitAPI.h"

#include <atomic>
#include <mutex>
#include <set>
#include <string_view>

#include "MaaUtils/Buffer/BufferTypes.hpp"
#include "Management/Credentials.h"
#include "Management/Operations.h"

namespace
{
using namespace maa::ios::management;
enum class Operation
{
    Discover,
    Pair,
    CheckConnection,
    Prepare
};

bool parse_options(Operation operation, const char* source, Options& options, std::string& error)
{
    const std::string_view text = source ? source : "{}";
    if (text.size() > 65536) {
        error = "iOS management options exceed 64 KiB";
        return false;
    }
    const auto value = Json::parse(text, nullptr, false);
    if (!value.is_object()) {
        error = "iOS management options must be a JSON object";
        return false;
    }
    std::set<std::string> allowed;
    if (operation == Operation::Discover) {
        allowed = { "timeout_ms", "usb_only", "wifi_only" };
    }
    else if (operation == Operation::Pair) {
        allowed = { "udid", "wifi", "repair", "timeout_ms" };
    }
    else if (operation == Operation::CheckConnection) {
        allowed = { "udid", "wifi", "port" };
    }
    else {
        allowed = { "udid", "wifi", "port", "cache_directory", "replace", "download_timeout_seconds", "install_timeout_seconds" };
    }
    for (auto iter = value.begin(); iter != value.end(); ++iter) {
        if (!allowed.contains(iter.key()) && iter.key() != "pairing_records") {
            error = "unknown iOS management option: " + iter.key();
            return false;
        }
    }
    auto string = [&](const char* key, std::string& out) {
        const auto it = value.find(key);
        if (it == value.end()) {
            return true;
        }
        if (!it->is_string()) {
            error = std::string(key) + " must be a string";
            return false;
        }
        out = it->get<std::string>();
        if (out.find('\0') != std::string::npos) {
            error = std::string(key) + " must not contain NUL";
            return false;
        }
        return true;
    };
    auto boolean = [&](const char* key, bool& out) {
        const auto it = value.find(key);
        if (it == value.end()) {
            return true;
        }
        if (!it->is_boolean()) {
            error = std::string(key) + " must be boolean";
            return false;
        }
        out = it->get<bool>();
        return true;
    };
    auto integer = [&](const char* key, int minimum, int maximum, int& out) {
        const auto it = value.find(key);
        if (it == value.end()) {
            return true;
        }
        if (!it->is_number_integer() || (it->is_number_unsigned() && it->get<uint64_t>() > uint64_t(maximum))) {
            error = std::string(key) + " must be an integer in range";
            return false;
        }
        const auto number = it->get<int64_t>();
        if (number < minimum || number > maximum) {
            error = std::string(key) + " is out of range";
            return false;
        }
        out = static_cast<int>(number);
        return true;
    };
    if (!parse_credentials(value, options.pairing_records, error)) {
        return false;
    }
    options.timeout_ms = operation == Operation::Pair ? 120000 : 3000;
    int port = options.port;
    if (!string("udid", options.udid) || !string("wifi", options.wifi) || !string("cache_directory", options.cache_directory)
        || !boolean("repair", options.repair) || !boolean("replace", options.replace) || !boolean("usb_only", options.usb_only)
        || !boolean("wifi_only", options.wifi_only) || !integer("port", 1, 65535, port)
        || !integer("timeout_ms", operation == Operation::Pair ? 1 : 0, operation == Operation::Pair ? 300000 : 60000, options.timeout_ms)
        || !integer("download_timeout_seconds", 1, 3600, options.download_timeout_seconds)
        || !integer("install_timeout_seconds", 1, 3600, options.install_timeout_seconds)) {
        return false;
    }
    options.port = static_cast<unsigned short>(port);
    if (options.usb_only && options.wifi_only) {
        error = "usb_only and wifi_only are mutually exclusive";
        return false;
    }
    if (operation == Operation::Pair && !options.wifi.empty() && options.wifi != "auto") {
        error = "wireless first pairing uses wifi=auto, not a device address";
        return false;
    }
    if (options.repair && !options.wifi.empty()) {
        error = "repair is only supported for USB pairing";
        return false;
    }
    if (value.contains("port") && (options.wifi.empty() || options.wifi == "auto")) {
        error = "port requires an explicit Wi-Fi address";
        return false;
    }
    return true;
}

MaaBool manage(
    Operation operation,
    const char* options_json,
    MaaIOSControlUnitManagementEventCallback event_callback,
    MaaIOSControlUnitManagementCancelCallback cancel_callback,
    void* userdata,
    MaaStringBuffer* output,
    MaaStringBuffer* credential_output = nullptr)
{
    if (operation == Operation::Pair && (!credential_output || output == credential_output)) {
        return false;
    }
    if (!output) {
        return false;
    }
    try {
        if (credential_output) {
            credential_output->clear();
        }
        Options options;
        std::string error;
        if (!parse_options(operation, options_json, options, error)) {
            output->set(Json({ { "success", false }, { "error", error }, { "cancelled", false } }).dump());
            return false;
        }
        if (operation == Operation::Pair && !options.wifi.empty() && !event_callback) {
            output->set(
                Json({ { "success", false }, { "error", "wireless pairing requires a PIN event callback" }, { "cancelled", false } })
                    .dump());
            return false;
        }
        std::atomic<bool> rejected { false }, callback_failed { false };
        std::mutex events;
        Callbacks callbacks;
        callbacks.cancelled = [&] {
            if (rejected.load()) {
                return true;
            }
            try {
                if (cancel_callback && cancel_callback(userdata)) {
                    rejected = true;
                    return true;
                }
                return false;
            }
            catch (...) {
                callback_failed = true;
                rejected = true;
                return true;
            }
        };
        if (event_callback) {
            callbacks.event = [&](const Json& event) {
                std::lock_guard lock(events);
                if (rejected.load()) {
                    return false;
                }
                try {
                    const auto text = event.dump(-1, ' ', false, Json::error_handler_t::replace);
                    const bool accepted = event_callback(text.c_str(), userdata) != 0;
                    if (!accepted) {
                        rejected = true;
                    }
                    return accepted;
                }
                catch (...) {
                    callback_failed = true;
                    rejected = true;
                    return false;
                }
            };
        }
        Json result;
        std::string credential;
        if (callbacks.stopped()) {
            result = { { "success", false }, { "error", "iOS management cancelled" } };
        }
        else {
            switch (operation) {
            case Operation::Discover:
                result = discover(options, callbacks);
                break;
            case Operation::Pair:
                result = pair(options, callbacks, credential);
                break;
            case Operation::CheckConnection:
                result = check_connection(options, callbacks);
                break;
            case Operation::Prepare:
                result = prepare(options, callbacks);
                break;
            }
        }
        // Cancellation after a successful side effect does not undo it. Preserve
        // success and operation state, while reporting the caller's cancellation.
        result["cancelled"] = result.value("cancelled", false) || rejected.load();
        if (callback_failed.load()) {
            result["success"] = false;
            result["error"] = "iOS management callback failed";
        }
        const bool success = result.value("success", false);
        output->set(result.dump(-1, ' ', false, Json::error_handler_t::replace));
        if (success && credential_output) {
            credential_output->set(std::move(credential));
        }
        return success;
    }
    catch (...) {
        try {
            output->set(Json(
                            { { "success", false },
                              { "error", "iOS management failed" },
                              { "cancelled", false },
                              { "device_may_have_changed", operation == Operation::Prepare } })
                            .dump());
        }
        catch (...) {
        }
        return false;
    }
}
} // namespace

extern "C"
{
    MaaBool MaaIOSControlUnitDiscover(
        const char* options,
        MaaIOSControlUnitManagementEventCallback event,
        MaaIOSControlUnitManagementCancelCallback cancel,
        void* userdata,
        MaaStringBuffer* result)
    {
        return manage(Operation::Discover, options, event, cancel, userdata, result);
    }

    MaaBool MaaIOSControlUnitPair(
        const char* options,
        MaaIOSControlUnitManagementEventCallback event,
        MaaIOSControlUnitManagementCancelCallback cancel,
        void* userdata,
        MaaStringBuffer* result,
        MaaStringBuffer* credential)
    {
        return manage(Operation::Pair, options, event, cancel, userdata, result, credential);
    }

    MaaBool MaaIOSControlUnitCheckConnection(
        const char* options,
        MaaIOSControlUnitManagementEventCallback event,
        MaaIOSControlUnitManagementCancelCallback cancel,
        void* userdata,
        MaaStringBuffer* result)
    {
        return manage(Operation::CheckConnection, options, event, cancel, userdata, result);
    }

    MaaBool MaaIOSControlUnitPrepare(
        const char* options,
        MaaIOSControlUnitManagementEventCallback event,
        MaaIOSControlUnitManagementCancelCallback cancel,
        void* userdata,
        MaaStringBuffer* result)
    {
        return manage(Operation::Prepare, options, event, cancel, userdata, result);
    }
}
