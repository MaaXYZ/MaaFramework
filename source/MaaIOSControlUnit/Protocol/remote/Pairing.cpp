#include "remote/Pairing.h"

#include "i18n/Translation.h"
#include "remote/Device.h"
#include "remote/PairingChannel.h"
#include "wifi/PairRecord.h"
#include "wifi/PairSetup.h"
#include "wifi/PairVerify.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <memory>
#include <stop_token>
#include <thread>
#include <utility>

namespace maa::ios::remote
{
namespace
{
constexpr std::string_view kPairingService = "com.apple.internal.dt.coredevice.untrusted.tunnelservice";

bool validate(const UsbPairingOptions& options, std::string& error)
{
    if (options.timeout_ms < 1 || options.timeout_ms > 300000) {
        error = MAA_IOS_TR("USB pairing timeout must be between 1 and 300000 ms");
        return false;
    }
    return true;
}

void progress(const UsbPairingOptions& options, std::string_view message)
{
    if (options.progress) {
        options.progress(message);
    }
}

bool cancelled(const UsbPairingOptions& options, std::string& error)
{
    try {
        if (!options.should_cancel || !options.should_cancel()) {
            return false;
        }
        error = MAA_IOS_TR("USB pairing cancelled");
    }
    catch (...) {
        error = MAA_IOS_TR("USB pairing cancellation callback failed");
    }
    return true;
}

// Per-reply timeouts remain unchanged. This monitor only observes cancellation
// and closes the service TCP endpoint via its existing stop callback.
class PairingCancellation
{
public:
    explicit PairingCancellation(std::function<bool()> should_cancel)
        : worker_([this, should_cancel = std::move(should_cancel)](std::stop_token stop) {
            while (!stop.stop_requested()) {
                bool cancelled = false;
                try {
                    cancelled = should_cancel && should_cancel();
                }
                catch (...) {
                    cancelled = true;
                }
                if (cancelled) {
                    source_.request_stop();
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
        })
    {
    }

    std::stop_token token() const { return source_.get_token(); }

    void stop() { source_.request_stop(); }

private:
    std::stop_source source_;
    std::jthread worker_;
};

struct UsbPairingChannel
{
    // carrier 引用 connection，connection 引用 Device 的 Stack；按逆序释放。
    std::optional<Device> device;
    std::unique_ptr<PairingCancellation> cancellation;
    std::unique_ptr<ServiceConnection> connection;
    std::unique_ptr<XpcPairingCarrier> carrier;
    std::unique_ptr<wifi::Rppairing> channel;
};

std::unique_ptr<UsbPairingChannel> open_pairing_channel(const std::string& udid, const UsbPairingOptions& options, std::string& error)
{
    if (cancelled(options, error)) {
        return nullptr;
    }
    transport::IoBudget budget({ std::chrono::seconds(30), options.should_cancel });
    auto opened = std::make_unique<UsbPairingChannel>();
    opened->device = Device::establish_bounded(udid, budget, error, false, true);
    if (!opened->device) {
        return nullptr;
    }
    // 除选择阶段的过滤，再检查实际连接归属；热插拔变化不能降级为网络首次信任。
    if (opened->device->udid() != udid || opened->device->connection_type() != "USB") {
        error = MAA_IOS_TR("Pairing requires the selected device to remain connected over USB");
        return nullptr;
    }
    const auto service = opened->device->rsd().service(kPairingService);
    if (!service) {
        error = MAA_IOS_TR("Device does not provide the USB RemotePairing service");
        return nullptr;
    }
    if (!service->uses_remote_xpc) {
        error = MAA_IOS_TR("USB RemotePairing service does not support RemoteXPC");
        return nullptr;
    }
    opened->cancellation = std::make_unique<PairingCancellation>(options.should_cancel);
    transport::BudgetWatchdog preparation(budget, [&] {
        opened->cancellation->stop();
        opened->device->interrupt_startup();
    });
    opened->connection = opened->device->connect(kPairingService, error, false, opened->cancellation->token());
    if (!opened->connection) {
        return nullptr;
    }

    // 设备可能先发送 ServiceVersion。超时允许继续握手，连接断开则不能继续。
    // 版本消息不包含配对信封；未发请求前出现其它消息属于意外协议状态。
    xpc::Value initial;
    const auto status = opened->connection->wait_message(initial, std::min(options.timeout_ms, 5000), error);
    if (status == Channel::Wait::Broken) {
        return nullptr;
    }
    if (status == Channel::Wait::Message && !initial.find("ServiceVersion")) {
        error = MAA_IOS_TR("Unexpected initial USB RemotePairing message");
        return nullptr;
    }
    if (!preparation.finish(error) || cancelled(options, error)) {
        return nullptr;
    }
    error.clear();
    opened->carrier = std::make_unique<XpcPairingCarrier>(*opened->connection, options.timeout_ms);
    opened->channel = std::make_unique<wifi::Rppairing>(*opened->carrier);
    return opened;
}
} // namespace

std::optional<transport::DeviceRecord>
    detail::select_pairing_usb_device(const std::vector<transport::DeviceRecord>& devices, std::string_view udid, std::string& error)
{
    error.clear();
    std::optional<transport::DeviceRecord> selected;
    for (const auto& device : devices) {
        if (!device.is_usb() || (!udid.empty() && device.udid != udid)) {
            continue;
        }
        if (selected) {
            error = MAA_IOS_TR("Multiple USB devices match; select one by UDID");
            return std::nullopt;
        }
        selected = device;
    }
    if (!selected) {
        error = MAA_IOS_TR("No matching USB device is connected");
    }
    else if (selected->udid.empty()) {
        error = MAA_IOS_TR("USB device has no UDID; cannot bind a pairing record");
        return std::nullopt;
    }
    return selected;
}

PairingResult detail::run_usb_pairing_workflow(
    std::string_view selected_udid,
    std::string_view hostname,
    const UsbPairingOptions& options,
    const PairingWorkflowOperations& operations)
{
    PairingResult result;
    result.udid = selected_udid;
    if (!validate(options, result.error) || cancelled(options, result.error)) {
        return result;
    }
    if (selected_udid.empty() || (!options.udid.empty() && options.udid != selected_udid)) {
        result.error = MAA_IOS_TR("USB pairing device selection is invalid");
        return result;
    }
    if (!operations.verify || !operations.setup) {
        result.error = MAA_IOS_TR("USB pairing workflow operations are unavailable");
        return result;
    }

    try {
        std::optional<wifi::PairRecord> existing = options.existing_record;
        for (const auto& candidate : options.existing_records) {
            if (candidate.udid != selected_udid) {
                continue;
            }
            if (existing) {
                result.error = "Multiple credentials match the USB device";
                return result;
            }
            existing = candidate;
        }
        if (existing && existing->udid != result.udid) {
            result.error = "Supplied pairing credential belongs to another device";
            return result;
        }
        if (existing) {
            std::string error;
            if (existing->complete() && existing->has_peer_identity()) {
                progress(options, MAA_IOS_TR("Verifying the existing USB pairing record"));
                if (cancelled(options, result.error)) {
                    return result;
                }
                const auto verified = operations.verify(*existing, error);
                if (cancelled(options, result.error)) {
                    return result;
                }
                if (verified.outcome == wifi::VerifyOutcome::Paired) {
                    result.ok = true;
                    result.reused = true;
                    result.record = *existing;
                    return result;
                }
                if (verified.outcome != wifi::VerifyOutcome::NotPaired) {
                    result.error =
                        MAA_IOS_TR("Existing pairing record verification failed: ") + (verified.error.empty() ? error : verified.error);
                    return result;
                }
                if (!options.allow_repair) {
                    result.error = MAA_IOS_TR("Device rejected the pairing record; enable USB repair explicitly");
                    return result;
                }
            }
            else if (existing && !options.allow_repair) {
                result.error = MAA_IOS_TR("Existing pairing record is incomplete; enable USB repair explicitly");
                return result;
            }
        }

        if (hostname.empty()) {
            result.error = MAA_IOS_TR("Cannot obtain the local hostname for USB pairing");
            return result;
        }
        // Independent applications must not replace each other's pairing identity
        // merely because they run on the same computer. Repairs keep the identity.
        std::string host_identifier;
        if (existing) {
            host_identifier = existing->host_identifier;
        }
        else {
            const auto nonce = wifi::random_bytes(16, result.error);
            if (!nonce) {
                return result;
            }
            host_identifier = wifi::host_identifier_uuid3(wifi::b64_encode(*nonce));
        }
        if (host_identifier.empty()) {
            result.error = MAA_IOS_TR("Cannot generate a host identifier for USB pairing");
            return result;
        }
        progress(options, MAA_IOS_TR("Starting USB pairing; confirm the request on the device"));
        std::string error;
        if (cancelled(options, result.error)) {
            return result;
        }
        const auto setup = operations.setup(host_identifier, hostname, selected_udid, error);
        if (cancelled(options, result.error)) {
            return result;
        }
        if (!setup.ok) {
            result.error = MAA_IOS_TR("USB pair setup failed: ") + (setup.error.empty() ? error : setup.error);
            return result;
        }
        if (setup.record.udid != selected_udid || setup.record.host_identifier != host_identifier || !setup.record.complete()
            || !setup.record.has_peer_identity()) {
            result.error = MAA_IOS_TR("USB pair setup returned an incomplete or mismatched record");
            return result;
        }
        progress(options, MAA_IOS_TR("Reconnecting to verify the new USB pairing record"));
        if (cancelled(options, result.error)) {
            return result;
        }
        const auto verified = operations.verify(setup.record, error);
        if (cancelled(options, result.error)) {
            return result;
        }
        if (verified.outcome != wifi::VerifyOutcome::Paired) {
            result.error = MAA_IOS_TR("New USB pairing record verification failed: ") + (verified.error.empty() ? error : verified.error);
            return result;
        }
        result.ok = true;
        result.record = setup.record;
        return result;
    }
    catch (const std::exception&) {
        // 不转发 callback/文件异常的 what()，避免意外泄露配对载体或记录内容。
        result.error = MAA_IOS_TR("USB pairing workflow failed before completion");
        return result;
    }
}

PairingResult pair_usb_remote(const UsbPairingOptions& options)
{
    PairingResult result;
    if (!validate(options, result.error) || cancelled(options, result.error)) {
        return result;
    }
    std::string error;
    std::vector<transport::DeviceRecord> devices;
    const auto status =
        transport::Usbmux::list_devices_for_discovery(devices, std::chrono::milliseconds(1000), options.should_cancel, error);
    if (status != transport::UsbmuxDiscoveryStatus::complete) {
        if (cancelled(options, result.error)) {
            return result;
        }
        if (error.empty()) {
            error = MAA_IOS_TR("USB pairing enumeration timed out or service is unavailable");
        }
        result.error = MAA_IOS_TR("Cannot enumerate devices for USB pairing: ") + error;
        return result;
    }
    const auto selected = detail::select_pairing_usb_device(devices, options.udid, error);
    if (!selected) {
        result.error = std::move(error);
        return result;
    }

    detail::PairingWorkflowOperations operations;
    operations.verify = [&](const wifi::PairRecord& record, std::string& operation_error) {
        auto opened = open_pairing_channel(selected->udid, options, operation_error);
        if (!opened) {
            wifi::PairVerifyResult failed;
            failed.error = operation_error;
            return failed;
        }
        return wifi::pair_verify(*opened->channel, record, operation_error);
    };
    operations.setup =
        [&](std::string_view host_identifier, std::string_view hostname, std::string_view udid, std::string& operation_error) {
            auto opened = open_pairing_channel(selected->udid, options, operation_error);
            if (!opened) {
                wifi::PairSetupResult failed;
                failed.error = operation_error;
                return failed;
            }
            wifi::PairSetupOptions setup_options;
            setup_options.probe_verify_first = false;
            setup_options.pairing_kind = "setupManualPairing";
            return wifi::pair_setup(*opened->channel, host_identifier, hostname, udid, options.progress, setup_options, operation_error);
        };
    return detail::run_usb_pairing_workflow(selected->udid, wifi::local_hostname(), options, operations);
}
} // namespace maa::ios::remote
