#include "Operations.h"
#include "ddi/Cryptex.h"
#include "ddi/Mounter.h"
#include "ddi/ServiceTransport.h"
#include "ddi/Tss.h"
#include "remote/DeviceConnection.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <ostream>

namespace maa::ios::management
{
Json prepare(const Options& o, const Callbacks& callbacks)
{
    using namespace std::chrono;
    const auto& cancelled = callbacks.cancelled;
    std::string error;
    Json prepared;
    bool installation_started = false;
    auto fail = [&]() -> Json {
        return { { "success", false },
                 { "error", error },
                 { "cancelled", callbacks.stopped() },
                 { "device_may_have_changed", installation_started } };
    };
    ddi::DownloadTarget target;
    std::optional<uint64_t> ecid;
    std::optional<transport::DeviceRecord> record;
    std::optional<transport::Lockdown> lockdown;
    std::optional<remote::Device> device;
    if (o.wifi.empty()) {
        transport::IoBudget preparation({ seconds(30), cancelled });
        std::vector<transport::DeviceRecord> records;
        if (transport::Usbmux::list_devices_for_discovery(records, milliseconds(1000), cancelled, error)
            != transport::UsbmuxDiscoveryStatus::complete) {
            return fail();
        }
        record = remote::detail::select_usbmux_device(records, o.udid, error, true);
        if (!record) {
            return fail();
        }
        lockdown = transport::Lockdown::establish_bounded(record->device_id, record->udid, preparation, error);
        if (!lockdown) {
            return fail();
        }
        auto query = plist::Value::Dict();
        query.set("Request", plist::Value::Str("GetValue"));
        plist::Value reply;
        if (!lockdown->request_bounded(query, reply, preparation.remaining(), [&preparation] { return preparation.stopped(); }, error)) {
            return fail();
        }
        if (!preparation.check(error)) {
            return fail();
        }
        const auto* values = reply.find("Value");
        if (!values || !values->is_dict() || reply.find("Error")) {
            error = "DDI target metadata is missing its OS version or product model";
            return fail();
        }
        auto text = [&](const char* key) {
            const auto* v = values->find(key);
            return v ? v->as_string_or() : std::string { };
        };
        auto number = [&](const char* key) -> std::optional<uint64_t> {
            const auto* v = values->find(key);
            if (v && (v->is_uint() || (v->is_int() && v->integer >= 0))) {
                return v->as_uint_or();
            }
            return std::nullopt;
        };
        target.system_version = text("ProductVersion");
        target.product_type = text("ProductType");
        target.hardware_model = text("HardwareModel");
        target.board_id = number("BoardId");
        target.chip_id = number("ChipID");
        ecid = number("UniqueChipID");
    }
    else {
        device = remote::open_device(o.udid, o.wifi, error, o.port, cancelled, o.pairing_records);
        if (!device) {
            return fail();
        }
        target.system_version = device->property("OSVersion");
        target.product_type = device->property("ProductType");
        target.hardware_model = device->property("HardwareModel");
        const auto* properties = device->rsd().properties();
        auto number = [&](const char* key) -> std::optional<uint64_t> {
            const auto* v = properties ? properties->find(key) : nullptr;
            if (v && v->type == xpc::Type::UInt64) {
                return v->uint64;
            }
            if (v && v->type == xpc::Type::Int64 && v->int64 >= 0) {
                return static_cast<uint64_t>(v->int64);
            }
            return std::nullopt;
        };
        target.board_id = number("BoardId");
        target.chip_id = number("ChipID");
        ecid = number("UniqueChipID");
    }
    if (cancelled && cancelled()) {
        error = "DDI installation cancelled";
        return fail();
    }
    if (target.product_type.empty()) {
        error = "DDI target metadata is missing its OS version or product model";
        return fail();
    }
    auto selected = ddi::select_for_version(target.system_version, error);
    if (!selected) {
        return fail();
    }
    // A mounted image alone does not establish compatibility. On CoreDevice
    // systems also require the controller's screenshot feature to be advertised.
    if (!o.replace) {
        if (selected->kind != ddi::Kind::Classic && !device) {
            transport::IoBudget preparation({ seconds(30), cancelled });
            device = remote::Device::establish_bounded(record->udid, preparation, error, false, true);
            if (!device) {
                return fail();
            }
        }
        if (selected->kind == ddi::Kind::Classic
            || device->rsd().supports("com.apple.coredevice.screencaptureservice", "com.apple.coredevice.feature.capturescreenshot")) {
            auto adapter = !o.wifi.empty() ? ddi::ServiceTransport::open_remote(*device, seconds(30), cancelled, error)
                                           : ddi::ServiceTransport::open_usb(*lockdown, record->device_id, seconds(30), cancelled, error);
            if (!adapter) {
                return fail();
            }
            auto wire = adapter->service();
            ddi::MountOptions inspect;
            inspect.timeout = seconds(30);
            inspect.cancelled = cancelled;
            auto mounted = ddi::inspect_mounted(wire, target, inspect, error);
            if (!mounted) {
                return fail();
            }
            if (mounted->state != ddi::MountState::NotMounted
                && (selected->kind != ddi::Kind::Classic || mounted->signature_matches == true)) {
                return { { "success", true },
                         { "error", "" },
                         { "cancelled", callbacks.stopped() },
                         { "status", "already_mounted" },
                         { "downloaded", false },
                         { "device_may_have_changed", false } };
            }
        }
    }
    callbacks.notify(
        { { "type", "progress" },
          { "phase", "target" },
          { "message", "DDI target: OS " + target.system_version + ", model " + target.product_type },
          { "os_version", target.system_version },
          { "product_type", target.product_type },
          { "image_kind", ddi::kind_name(selected->kind) } });
    ddi::DownloadOptions download;
    download.cancelled = cancelled;
    download.timeout = seconds(o.download_timeout_seconds);
    if (!o.cache_directory.empty()) {
        std::error_code ec;
        download.cache_root =
            std::filesystem::absolute(std::filesystem::path(std::u8string(o.cache_directory.begin(), o.cache_directory.end())), ec);
        if (ec) {
            error = ec.message();
            return fail();
        }
    }
    std::string last_file;
    uint64_t last_bytes = 0;
    auto progress = [&](const std::string& file, uint64_t received, uint64_t total) {
        if (last_file != file || received == total || received >= last_bytes + 1024 * 1024) {
            callbacks.notify(
                { { "type", "progress" },
                  { "phase", "transfer" },
                  { "file", file },
                  { "received", received },
                  { "total", total },
                  { "message", "DDI transfer: " + file + " (" + std::to_string(received) + "/" + std::to_string(total) + " bytes)" } });
            last_file = file;
            last_bytes = received;
        }
    };
    download.progress = progress;
    auto image = ddi::download_for_target(download, target, error);
    if (!image) {
        return fail();
    }
    callbacks.notify(
        { { "type", "progress" },
          { "phase", "source" },
          { "source_url", image->source_url },
          { "message", "DDI source: " + image->source_url } });
    // Establishing the CoreDevice tunnel is connection preparation. The
    // installation budget starts once that transport is available.
    if (selected->kind == ddi::Kind::Cryptex && !device) {
        transport::IoBudget preparation({ seconds(30), cancelled });
        device = remote::Device::establish_bounded(record->udid, preparation, error, false, true);
        if (!device) {
            return fail();
        }
    }
    if (cancelled && cancelled()) {
        error = "DDI installation cancelled";
        return fail();
    }
    const auto deadline = steady_clock::now() + seconds(o.install_timeout_seconds);
    auto remaining = [&] {
        return duration_cast<milliseconds>(deadline - steady_clock::now());
    };
    auto stopped = [&] {
        return (cancelled && cancelled()) || remaining() <= milliseconds::zero();
    };
    auto service = [&](milliseconds timeout) -> std::unique_ptr<ddi::ServiceTransport> {
        timeout = std::min(timeout, remaining());
        if (device && !o.wifi.empty()) {
            return ddi::ServiceTransport::open_remote(*device, timeout, stopped, error);
        }
        return ddi::ServiceTransport::open_usb(*lockdown, record->device_id, timeout, stopped, error);
    };
    if (selected->kind == ddi::Kind::Cryptex) {
        ddi::CryptexInstallOptions options;
        options.timeout = seconds(o.install_timeout_seconds);
        options.cancelled = stopped;
        options.replace_existing = o.replace;
        options.progress = progress;
        options.personalized_mounted = [&](milliseconds timeout, std::string& out) -> std::optional<bool> {
            auto adapter = service(timeout);
            if (!adapter) {
                out = error;
                return std::nullopt;
            }
            auto wire = adapter->service();
            ddi::MountOptions inspect;
            inspect.timeout = seconds(o.install_timeout_seconds);
            inspect.cancelled = stopped;
            auto result = ddi::inspect_mounted(wire, ddi::Kind::Personalized, inspect, out);
            if (!result) {
                return std::nullopt;
            }
            return result->state != ddi::MountState::NotMounted;
        };
        options.remove_personalized = [&](milliseconds timeout, std::string& out) {
            auto adapter = service(timeout);
            if (!adapter) {
                out = error;
                return false;
            }
            auto wire = adapter->service();
            ddi::MountOptions inspect;
            inspect.timeout = seconds(o.install_timeout_seconds);
            inspect.cancelled = stopped;
            return ddi::unmount_current(wire, ddi::Kind::Personalized, inspect, out);
        };
        installation_started = true;
        auto result = ddi::install_cryptex(*device, *image, target, options, error);
        if (!result) {
            return fail();
        }
        auto adapter = service(remaining());
        if (!adapter) {
            return fail();
        }
        auto wire = adapter->service();
        ddi::MountOptions verify;
        verify.timeout = seconds(o.install_timeout_seconds);
        verify.cancelled = stopped;
        const auto mounted = ddi::inspect_mounted(wire, ddi::Kind::Cryptex, verify, error);
        if (!mounted) {
            return fail();
        }
        if (mounted->state == ddi::MountState::NotMounted) {
            error = "DDI Cryptex is installed but its developer image is not mounted";
            return fail();
        }
        prepared = { { "status", result->already_installed ? "already_installed" : "installed" },
                     { "identifier", result->identifier },
                     { "version", result->version } };
    }
    else {
        if (selected->kind == ddi::Kind::Personalized && !ecid) {
            error = "DDI installation requires the device ECID";
            return fail();
        }
        auto adapter = service(remaining());
        if (!adapter) {
            return fail();
        }
        auto wire = adapter->service();
        ddi::MountOptions options;
        options.timeout = seconds(o.install_timeout_seconds);
        options.cancelled = stopped;
        options.replace_existing = o.replace;
        options.progress = [&](uint64_t current, uint64_t total) {
            progress("DeveloperDiskImage.dmg", current, total);
        };
        options.personalize = [&](const plist::Value& manifest,
                                  const plist::Value& identifiers,
                                  uint64_t id,
                                  const std::vector<uint8_t>& nonce,
                                  milliseconds timeout,
                                  std::string& out) {
            auto request = ddi::build_personalized_tss_request(manifest, identifiers, id, nonce, out);
            if (!request) {
                return std::optional<std::vector<uint8_t>> { };
            }
            ddi::TssOptions tss;
            tss.timeout = std::min(timeout, remaining());
            tss.cancelled = stopped;
            return ddi::request_tss_ticket(*request, tss, out);
        };
        installation_started = true;
        auto result = ddi::mount_image(wire, *image, target, ecid.value_or(0), options, error);
        if (!result) {
            return fail();
        }
        prepared = { { "status", result->state == ddi::MountState::AlreadyMounted ? "already_mounted" : "mounted" },
                     { "image_type", result->image_type } };
    }
    prepared["success"] = true;
    prepared["error"] = "";
    prepared["cancelled"] = false;
    prepared["device_may_have_changed"] = installation_started;
    prepared["os_version"] = target.system_version;
    prepared["product_type"] = target.product_type;
    prepared["image_kind"] = ddi::kind_name(selected->kind);
    prepared["source_url"] = image->source_url;
    return prepared;
}
}
