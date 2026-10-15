#include "remote/Discovery.h"

#include "i18n/Translation.h"
#include "transport/Usbmux.h"
#include "wifi/DiscoveryIdentity.h"
#include "wifi/Mdns.h"
#include "wifi/PairRecord.h"

#include <algorithm>
#include <exception>
#include <map>
#include <tuple>
#include <utility>

namespace maa::ios::remote
{
namespace
{

void warn(std::vector<std::string>& warnings, std::string message)
{
    if (warnings.size() < 32 && std::find(warnings.begin(), warnings.end(), message) == warnings.end()) {
        warnings.push_back(std::move(message));
    }
}

void append_candidate(DiscoveredDevice& device, DiscoveryCandidate candidate)
{
    if (std::find(device.candidates.begin(), device.candidates.end(), candidate) == device.candidates.end()) {
        device.candidates.push_back(std::move(candidate));
    }
}

DiscoveryPairing pairing_status(const wifi::DiscoveryIdentityMatch& match, const std::vector<wifi::PairRecord>& records)
{
    switch (match.status) {
    case wifi::DiscoveryIdentityStatus::matched: {
        const auto& record = records[*match.record_index];
        return record.complete() && record.has_peer_identity() ? DiscoveryPairing::ready : DiscoveryPairing::needs_pairing;
    }
    case wifi::DiscoveryIdentityStatus::ambiguous:
        return DiscoveryPairing::ambiguous;
    case wifi::DiscoveryIdentityStatus::invalid_advertisement:
        return DiscoveryPairing::invalid_advertisement;
    case wifi::DiscoveryIdentityStatus::crypto_error:
        return DiscoveryPairing::crypto_error;
    case wifi::DiscoveryIdentityStatus::unmatched:
        return match.identifier_hints.empty() ? DiscoveryPairing::unmatched : DiscoveryPairing::identifier_hint;
    }
    return DiscoveryPairing::unmatched;
}

} // namespace

DiscoveryResult detail::merge_discovery(
    const std::vector<transport::DeviceRecord>& usb_records,
    const std::vector<wifi::mdns::Advertisement>& advertisements,
    const std::vector<wifi::PairRecord>& pair_records)
{
    DiscoveryResult result;
    std::map<std::string, size_t> known_devices;
    std::map<std::tuple<std::string, std::string, std::string>, size_t> unknown_devices;
    auto known_device = [&](const std::string& udid) -> DiscoveredDevice& {
        const auto [it, inserted] = known_devices.emplace(udid, result.devices.size());
        if (inserted) {
            result.devices.push_back({ udid, { }, { } });
        }
        return result.devices[it->second];
    };

    for (const auto& record : usb_records) {
        // usbmux 正常记录应有 UDID。异常的空值不能把不同 device_id 合并成一台。
        DiscoveredDevice* device = nullptr;
        if (record.udid.empty()) {
            result.devices.push_back({ });
            device = &result.devices.back();
        }
        else {
            device = &known_device(record.udid);
        }
        DiscoveryCandidate candidate;
        candidate.transport = DiscoveryTransport::usbmux;
        candidate.connection_type = record.connection_type;
        candidate.device_id = record.device_id;
        append_candidate(*device, std::move(candidate));
    }

    for (const auto& advertisement : advertisements) {
        const auto match = wifi::match_advertisement(advertisement.identifier, advertisement.auth_tag, pair_records);
        const auto pairing = pairing_status(match, pair_records);
        std::string udid;
        if (match.status == wifi::DiscoveryIdentityStatus::matched && match.record_index) {
            udid = pair_records[*match.record_index].udid;
        }

        DiscoveredDevice* device = nullptr;
        if (!udid.empty()) {
            device = &known_device(udid);
        }
        else {
            // 尚未匹配本地记录时，不能把同一实例/identifier 下互相冲突的 authTag
            // 合并为一台设备；相同端点也不能消除这条冲突。identifier/tag 保留原始字节。
            const auto key = std::make_tuple(advertisement.instance, advertisement.identifier, advertisement.auth_tag);
            const auto [it, inserted] = unknown_devices.emplace(key, result.devices.size());
            if (inserted) {
                result.devices.push_back({ });
            }
            device = &result.devices[it->second];
        }
        if (device->name.empty() && !advertisement.name.empty()) {
            device->name = advertisement.name;
        }
        if (pairing == DiscoveryPairing::crypto_error) {
            warn(result.warnings, MAA_IOS_TR("Cannot match mDNS advertisements to pairing records"));
        }

        DiscoveryCandidate candidate;
        candidate.transport = DiscoveryTransport::remote_pairing;
        candidate.connection_type = "RemotePairing";
        candidate.instance = advertisement.instance;
        candidate.identifier = advertisement.identifier;
        candidate.pairing = pairing;
        if (advertisement.endpoints.empty()) {
            // PTR/TXT 已发现而 SRV/A/AAAA 尚未到齐，也保留这条发现线索。
            append_candidate(*device, std::move(candidate));
        }
        else {
            for (const auto& endpoint : advertisement.endpoints) {
                candidate.address = endpoint.address;
                candidate.port = endpoint.port;
                candidate.interface_index = endpoint.interface_index;
                candidate.interface_name = endpoint.interface_name;
                append_candidate(*device, candidate);
            }
        }
    }
    return result;
}

DiscoveryResult discover_devices(const DiscoveryOptions& options)
{
    DiscoveryResult result;
    const auto stop_requested = [&] {
        if (!result.cancelled && options.should_cancel) {
            result.cancelled = options.should_cancel();
        }
        return result.cancelled;
    };
    if (stop_requested()) {
        result.cancelled = true;
        return result;
    }
    if (options.timeout.count() < 0 || options.timeout > std::chrono::seconds(60) || options.usb_timeout.count() < 0
        || options.usb_timeout > std::chrono::seconds(60)) {
        warn(result.warnings, MAA_IOS_TR("Device discovery timeout must be between 0 and 60000 ms"));
        return result;
    }

    std::vector<transport::DeviceRecord> usb_records;
    std::vector<wifi::mdns::Advertisement> advertisements;
    std::vector<wifi::PairRecord> pair_records;
    if (options.include_usb) {
        std::string error;
        auto status = transport::UsbmuxDiscoveryStatus::unavailable;
        try {
            status = transport::Usbmux::list_devices_for_discovery(usb_records, options.usb_timeout, stop_requested, error);
        }
        catch (const std::exception&) {
            usb_records.clear();
        }
        // 取消是用户结束本次快照，不作为 USB 来源失败继续扫描。
        result.usb_available = status == transport::UsbmuxDiscoveryStatus::complete;
        if (status == transport::UsbmuxDiscoveryStatus::cancelled || stop_requested()) {
            result.cancelled = true;
            // 完整 USB 回复若已到达，保留它；取消时仍不读配对文件或启动 mDNS。
            result.devices = detail::merge_discovery(usb_records, { }, { }).devices;
            return result;
        }
        if (!result.usb_available) {
            warn(result.warnings, MAA_IOS_TR("USB device enumeration is unavailable"));
        }
    }
    if (stop_requested()) {
        result.cancelled = true;
    }
    if (options.include_wifi && options.timeout.count() > 0 && !result.cancelled) {
        // 先读取本地匹配材料，扫描中取消后仍能识别已收到的广播。
        // 不把没有本次广播的离线记录列为设备。
        pair_records = options.pairing_records;
        if (!result.cancelled) {
            auto browse = wifi::mdns::browse({ options.timeout, options.should_cancel });
            advertisements = std::move(browse.advertisements);
            result.wifi_available = browse.available;
            result.cancelled = browse.cancelled;
            for (auto& warning : browse.warnings) {
                warn(result.warnings, std::move(warning));
            }
        }
    }
    auto merged = detail::merge_discovery(usb_records, advertisements, pair_records);
    result.devices = std::move(merged.devices);
    for (auto& warning : merged.warnings) {
        warn(result.warnings, std::move(warning));
    }
    if (stop_requested()) {
        result.cancelled = true;
    }
    return result;
}

} // namespace maa::ios::remote
