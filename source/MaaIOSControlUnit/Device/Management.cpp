#include "ControlUnit/IOSDeviceAPI.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <set>
#include <string_view>

#include "MaaUtils/Buffer/BufferTypes.hpp"
#include "Operations.h"
#include "PairingConversion.h"
#include "wifi/Credentials.h"

namespace
{
using namespace maa::ios::management;

constexpr const char* kMissingPinCallback = "wireless pairing requires a PIN event callback";
constexpr const char* kMissingWiFiPairing = "Wi-Fi connection requires pairing credentials";

Json failure(std::string_view error)
{
    return { { "success", false }, { "error", std::string(error) }, { "cancelled", false } };
}

struct OwnedPairing : MaaIOSWiFiPairing
{
    explicit OwnedPairing(maa::ios::wifi::PairRecord record)
        : MaaIOSWiFiPairing { }
        , record_(std::move(record))
    {
        udid = record_.udid.c_str();
        host_identifier = record_.host_identifier.c_str();
        peer_identifier = record_.peer_identifier.c_str();
        std::copy(record_.host_private_key.begin(), record_.host_private_key.end(), host_private_key);
        std::copy(record_.host_public_key.begin(), record_.host_public_key.end(), host_public_key);
        std::copy(record_.peer_public_key.begin(), record_.peer_public_key.end(), peer_public_key);
        has_peer_alt_irk = !record_.peer_alt_irk.empty();
        if (has_peer_alt_irk) {
            std::copy(record_.peer_alt_irk.begin(), record_.peer_alt_irk.end(), peer_alt_irk);
        }
    }

private:
    maa::ios::wifi::PairRecord record_;
};

std::unique_ptr<OwnedPairing> make_pairing(maa::ios::wifi::PairRecord record)
{
    std::string error;
    if (!maa::ios::wifi::validate_credential(record, error)) {
        return nullptr;
    }
    return std::make_unique<OwnedPairing>(std::move(record));
}

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
    const auto parsed = maa::ios::json::parse(text);
    if (!parsed || !parsed->is_object()) {
        error = "iOS management options must be a JSON object";
        return false;
    }
    const auto& value = *parsed;
    std::set<std::string> allowed;
    if (operation == Operation::Discover) {
        allowed = { "timeout_ms" };
    }
    else if (operation == Operation::Pair) {
        allowed = { "timeout_ms", "host_name" };
    }
    else if (operation == Operation::CheckConnection) {
        allowed = { "port" };
    }
    else {
        allowed = { "port", "cache_directory", "replace", "download_timeout_seconds", "install_timeout_seconds" };
    }
    for (const auto& [key, item] : value.as_object()) {
        if (!allowed.contains(key)) {
            error = "unknown iOS management option: " + key;
            return false;
        }
    }
    auto string = [&](const char* key, std::string& out) {
        const auto* it = value.find_value(key);
        if (!it) {
            return true;
        }
        if (!it->is_string()) {
            error = std::string(key) + " must be a string";
            return false;
        }
        out = it->as_string();
        if (out.find('\0') != std::string::npos) {
            error = std::string(key) + " must not contain NUL";
            return false;
        }
        return true;
    };
    auto boolean = [&](const char* key, bool& out) {
        const auto* it = value.find_value(key);
        if (!it) {
            return true;
        }
        if (!it->is_boolean()) {
            error = std::string(key) + " must be boolean";
            return false;
        }
        out = it->as_boolean();
        return true;
    };
    auto integer = [&](const char* key, int minimum, int maximum, int& out) {
        const auto* it = value.find_value(key);
        if (!it) {
            return true;
        }
        if (!it->is<int64_t>()) {
            error = std::string(key) + " must be an integer in range";
            return false;
        }
        const auto number = it->as<int64_t>();
        if (number < minimum || number > maximum) {
            error = std::string(key) + " is out of range";
            return false;
        }
        out = static_cast<int>(number);
        return true;
    };
    options.timeout_ms = operation == Operation::Pair ? 120000 : 3000;
    int port = options.port;
    if (!string("cache_directory", options.cache_directory) || !string("host_name", options.host_name)
        || !boolean("replace", options.replace) || !integer("port", 1, 65535, port)
        || !integer("timeout_ms", operation == Operation::Pair ? 1 : 0, operation == Operation::Pair ? 300000 : 60000, options.timeout_ms)
        || !integer("download_timeout_seconds", 1, 3600, options.download_timeout_seconds)
        || !integer("install_timeout_seconds", 1, 3600, options.install_timeout_seconds)) {
        return false;
    }
    options.port = static_cast<unsigned short>(port);
    if (value.contains("port") && (options.wifi.empty() || options.wifi == "auto")) {
        error = "port requires an explicit Wi-Fi address";
        return false;
    }
    return true;
}

bool copy_inputs(
    Operation operation,
    const char* udid,
    MaaIOSConnectionMethod method,
    const char* address,
    const MaaIOSWiFiPairing* const* pairings,
    MaaSize count,
    Options& options,
    std::string& error)
{
    if (method < MaaIOSConnectionMethod_Auto || method > MaaIOSConnectionMethod_WiFi) {
        error = "Invalid iOS connection method";
        return false;
    }
    options.connection_method = method;
    options.udid = udid ? udid : "";
    if ((operation == Operation::CheckConnection || operation == Operation::Prepare) && options.udid.empty()) {
        error = "iOS device UDID is required";
        return false;
    }
    if (options.udid.size() > 1024 || options.udid.find_first_of("\r\n") != std::string::npos) {
        error = "Invalid iOS device UDID";
        return false;
    }
    if (count > (operation == Operation::Discover ? 256U : 1U) || (count && !pairings)) {
        error = "Invalid iOS pairing list";
        return false;
    }
    if (method != MaaIOSConnectionMethod_USB) {
        options.wifi = address && *address ? address : "auto";
        std::set<std::string> identities;
        for (MaaSize i = 0; i < count; ++i) {
            if (!pairings[i]) {
                error = "NULL iOS pairing credential";
                return false;
            }
            auto record = maa::ios::wifi::copy_pairing(*pairings[i], error);
            if (!record) {
                return false;
            }
            if (!identities.insert(record->udid).second) {
                error = "iOS pairing credentials must have unique UDIDs";
                return false;
            }
            if (operation != Operation::Discover) {
                if (!options.udid.empty() && options.udid != record->udid) {
                    error = "iOS pairing credential UDID does not match the requested device";
                    return false;
                }
            }
            options.pairing_records.push_back(std::move(*record));
        }
        if (method == MaaIOSConnectionMethod_WiFi && operation != Operation::Discover && operation != Operation::Pair
            && options.pairing_records.empty()) {
            error = kMissingWiFiPairing;
            return false;
        }
    }
    return true;
}

MaaBool manage(
    const char* udid,
    MaaIOSConnectionMethod method,
    const char* address,
    const MaaIOSWiFiPairing* const* pairings,
    MaaSize count,
    Operation operation,
    const char* options_json,
    MaaToolkitIOSDeviceEventCallback event_callback,
    MaaToolkitIOSDeviceCancelCallback cancel_callback,
    void* userdata,
    MaaStringBuffer* output,
    MaaIOSWiFiPairing** credential_output = nullptr)
{
    if (credential_output) {
        *credential_output = nullptr;
    }
    if (operation == Operation::Pair && !credential_output) {
        return false;
    }
    if (!output) {
        return false;
    }
    try {
        Options options;
        std::string error;
        if (!copy_inputs(operation, udid, method, address, pairings, count, options, error)
            || !parse_options(operation, options_json, options, error)) {
            output->set(failure(error).to_string());
            return false;
        }
        if (operation == Operation::Pair && method == MaaIOSConnectionMethod_WiFi && !event_callback) {
            output->set(failure(kMissingPinCallback).to_string());
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
                    const auto text = maa::ios::json::write(event, true);
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
        std::optional<maa::ios::wifi::PairRecord> credential;
        if (callbacks.stopped()) {
            result = failure("iOS management cancelled");
        }
        else if (operation != Operation::Discover && !resolve_connection(options, callbacks, error)) {
            result = failure(error);
        }
        else if (operation == Operation::Pair && options.connection_method == MaaIOSConnectionMethod_WiFi && !event_callback) {
            result = failure(kMissingPinCallback);
        }
        else if (
            operation != Operation::Discover && operation != Operation::Pair && options.connection_method == MaaIOSConnectionMethod_WiFi
            && options.pairing_records.empty()) {
            result = failure(kMissingWiFiPairing);
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
        result["cancelled"] = result.get("cancelled", false) || rejected.load();
        if (callback_failed.load()) {
            result["success"] = false;
            result["error"] = "iOS management callback failed";
        }
        const bool success = result.get("success", false);
        output->set(maa::ios::json::write(result, true));
        if (success && credential_output && credential) {
            auto pairing = make_pairing(std::move(*credential));
            if (!pairing) {
                output->set(
                    ::json::object(
                        { { "success", false },
                          { "error", "Device pairing identifier cannot be represented as text" },
                          { "cancelled", false } })
                        .to_string());
                return false;
            }
            *credential_output = pairing.release();
        }
        return success;
    }
    catch (...) {
        try {
            output->set(
                ::json::object(
                    { { "success", false },
                      { "error", "iOS management failed" },
                      { "cancelled", false },
                      { "device_may_have_changed", operation == Operation::Prepare } })
                    .to_string());
        }
        catch (...) {
        }
        return false;
    }
}
} // namespace

namespace
{
class IOSDeviceMgr final : public MAA_CTRL_UNIT_NS::IOSDeviceAPI
{
public:
    MaaBool discover(
        MaaIOSConnectionMethod connection_method,
        const MaaIOSWiFiPairing* const* wifi_pairings,
        MaaSize wifi_pairing_count,
        const char* options,
        MaaToolkitIOSDeviceEventCallback event,
        MaaToolkitIOSDeviceCancelCallback cancel,
        void* userdata,
        MaaStringBuffer* result) override
    {
        return manage(
            nullptr,
            connection_method,
            nullptr,
            wifi_pairings,
            wifi_pairing_count,
            Operation::Discover,
            options,
            event,
            cancel,
            userdata,
            result);
    }

    MaaBool pair(
        const char* udid,
        MaaIOSConnectionMethod connection_method,
        const char* options,
        MaaToolkitIOSDeviceEventCallback event,
        MaaToolkitIOSDeviceCancelCallback cancel,
        void* userdata,
        MaaStringBuffer* result,
        MaaIOSWiFiPairing** credential) override
    {
        return manage(udid, connection_method, nullptr, nullptr, 0, Operation::Pair, options, event, cancel, userdata, result, credential);
    }

    MaaBool check_connection(
        const char* udid,
        MaaIOSConnectionMethod connection_method,
        const char* wifi_address,
        const MaaIOSWiFiPairing* wifi_pairing,
        const char* options,
        MaaToolkitIOSDeviceEventCallback event,
        MaaToolkitIOSDeviceCancelCallback cancel,
        void* userdata,
        MaaStringBuffer* result) override
    {
        return manage(
            udid,
            connection_method,
            wifi_address,
            &wifi_pairing,
            wifi_pairing ? 1 : 0,
            Operation::CheckConnection,
            options,
            event,
            cancel,
            userdata,
            result);
    }

    MaaBool prepare(
        const char* udid,
        MaaIOSConnectionMethod connection_method,
        const char* wifi_address,
        const MaaIOSWiFiPairing* wifi_pairing,
        const char* options,
        MaaToolkitIOSDeviceEventCallback event,
        MaaToolkitIOSDeviceCancelCallback cancel,
        void* userdata,
        MaaStringBuffer* result) override
    {
        return manage(
            udid,
            connection_method,
            wifi_address,
            &wifi_pairing,
            wifi_pairing ? 1 : 0,
            Operation::Prepare,
            options,
            event,
            cancel,
            userdata,
            result);
    }

    MaaIOSWiFiPairing* parse_pairing_record(const char* record) override
    {
        if (!record) {
            return nullptr;
        }
        try {
            std::string error;
            auto value = maa::ios::wifi::parse_credential(record, error);
            auto pairing = value ? make_pairing(std::move(*value)) : nullptr;
            return pairing.release();
        }
        catch (...) {
            return nullptr;
        }
    }

    MaaBool format_pairing_record(const MaaIOSWiFiPairing* pairing, MaaStringBuffer* record) override
    {
        if (!record) {
            return false;
        }
        try {
            record->clear();
            if (!pairing) {
                return false;
            }
            std::string error;
            auto value = maa::ios::wifi::copy_pairing(*pairing, error);
            if (!value) {
                return false;
            }
            record->set(maa::ios::wifi::format_record(*value));
            return true;
        }
        catch (...) {
            return false;
        }
    }

    void destroy_pairing(MaaIOSWiFiPairing* pairing) override { delete static_cast<OwnedPairing*>(pairing); }
};
} // namespace

extern "C" MAA_CTRL_UNIT_NS::IOSDeviceAPI* MaaIOSControlUnitGetDeviceAPI()
{
    static IOSDeviceMgr api;
    return &api;
}
