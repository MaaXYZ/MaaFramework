#include "remote/Device.h"

#include "MaaUtils/Logger.h"

#include <algorithm>
#include <map>
#include <memory>

#include "remote/RemoteXpc.h"
#include "transport/TcpConnect.h"
#include "wifi/PairVerify.h"
#include "wifi/RemotePairing.h"

namespace maa::ios::remote
{
namespace
{

/// USB 路径通过 lockdown 启动的包隧道入口服务。
constexpr const char* kCoreDeviceProxy = "com.apple.internal.devicecompute.CoreDeviceProxy";

/// verbose 模式报告建立会话的当前阶段，便于定位等待发生在哪一层。
void stage(bool verbose, const char* what)
{
    if (verbose) {
        LogDebug << std::format("[stage] {}", what);
    }
}

} // namespace

std::string proxy_failure_hint(const std::string_view lockdown_error)
{
    const std::string e(lockdown_error);
    // 先匹配明确的锁屏和信任错误，再补充服务能力或通用连接提示。
    if (e.find("PasswordProtected") != std::string::npos) {
        return " (device is locked; unlock it and keep the screen awake before retrying)";
    }
    if (e.find("UserDenied") != std::string::npos || e.find("Trust") != std::string::npos) {
        return " (computer not trusted; unlock the device and confirm Trust)";
    }
    if (e.find("InvalidService") != std::string::npos) {
        return " (service unavailable; check Developer Mode and a compatible DDI; download: MaaToolkitIOSDevicePrepare, installation: the "
               "MaaToolkit iOS documentation)";
    }
    return " (check DDI mounting and Developer Mode)";
}

std::string mask(std::string_view value, std::size_t keep)
{
    if (value.size() <= keep) {
        return std::string(value);
    }
    return "****" + std::string(value.substr(value.size() - keep));
}

std::optional<transport::DeviceRecord> detail::select_usbmux_device(
    const std::vector<transport::DeviceRecord>& records,
    std::string_view serial,
    std::string& error,
    bool usb_only)
{
    error.clear();
    std::map<std::string, std::vector<const transport::DeviceRecord*>> devices;
    bool missing_udid = false;
    for (const auto& record : records) {
        if (usb_only && !record.is_usb()) {
            continue;
        }
        if (record.udid.empty()) {
            missing_udid = true;
            continue;
        }
        devices[record.udid].push_back(&record);
    }
    if (devices.empty()) {
        if (missing_udid) {
            error = "Connected usbmux entries have no usable device UDID";
        }
        else {
            // usbmux 为空或 USB 过滤后为空时沿用原来的连接提示。
            error =
                "No USB device found (usbmux list is empty). Check the data cable and USB "
                "connection, then reconnect the device. Untrusted devices normally appear in "
                "this list too.";
        }
        return std::nullopt;
    }
    auto candidates = [&] {
        std::string value;
        for (const auto& [udid, unused] : devices) {
            value += " " + mask(udid);
        }
        return value;
    };
    auto selected = devices.begin();
    if (serial.empty()) {
        if (devices.size() > 1) {
            error = "Connected devices: " + std::to_string(devices.size()) + "; select one: " + candidates();
            return std::nullopt;
        }
    }
    else {
        selected = devices.find(std::string(serial));
        if (selected == devices.end()) {
            error = "No connected device matches " + mask(serial) + "; connected devices: " + candidates() + " (total "
                    + std::to_string(devices.size()) + "). Check the selected UDID and USB connection.";
            return std::nullopt;
        }
    }
    const auto& connections = selected->second;
    const bool has_usb = std::any_of(connections.begin(), connections.end(), [](const auto* record) { return record->is_usb(); });
    const transport::DeviceRecord* chosen = nullptr;
    for (const auto* record : connections) {
        if (has_usb && !record->is_usb()) {
            continue;
        }
        if (chosen) {
            error = "Cannot choose a unique usbmux connection for device " + mask(selected->first);
            return std::nullopt;
        }
        chosen = record;
    }
    return *chosen;
}

Device::Device(Device&&) noexcept = default;

Device& Device::operator=(Device&& other) noexcept
{
    if (this == &other) {
        return *this;
    }
    // 先按借用关系释放旧会话；逐成员默认赋值会在旧 RSD 释放前销毁其 Stack。
    rsd_.reset();
    stack_.reset();
    tunnel_.reset();
    lockdown_.reset();
    udid_ = std::move(other.udid_);
    connection_type_ = std::move(other.connection_type_);
    lockdown_ = std::move(other.lockdown_);
    tunnel_ = std::move(other.tunnel_);
    stack_ = std::move(other.stack_);
    rsd_ = std::move(other.rsd_);
    return *this;
}

Device::~Device() = default;

std::vector<transport::DeviceRecord> Device::list(std::string& err)
{
    std::vector<transport::DeviceRecord> out;
    auto mux = transport::Usbmux::open(err);
    if (!mux) {
        return out;
    }
    if (!mux->list_devices(out, err)) {
        out.clear();
    }
    return out;
}

std::optional<Device> Device::establish(std::string_view udid, std::string& err, bool verbose, bool usb_only)
{
    transport::IoBudget budget;
    return establish_bounded(udid, budget, err, verbose, usb_only);
}

std::optional<Device>
    Device::establish_bounded(std::string_view udid, const transport::IoBudget& budget, std::string& err, bool verbose, bool usb_only)
{
    if (!budget.check(err)) {
        return std::nullopt;
    }
    const auto cancelled = [&budget] {
        return budget.stopped();
    };
    auto mux = transport::Usbmux::open_for_service(budget.remaining(), cancelled, err);
    if (!mux) {
        budget.check(err);
        return std::nullopt;
    }
    std::vector<transport::DeviceRecord> devices;
    if (!mux->list_devices_for_service(devices, budget.remaining(), cancelled, err)) {
        budget.check(err);
        return std::nullopt;
    }
    const auto chosen = detail::select_usbmux_device(devices, udid, err, usb_only);
    if (!chosen) {
        return std::nullopt;
    }

    std::optional<Device> dev;
    dev.emplace();
    dev->udid_ = chosen->udid;
    dev->connection_type_ = chosen->connection_type;

    stage(verbose, "lockdown pairing session");
    dev->lockdown_ = transport::Lockdown::establish_bounded(chosen->device_id, chosen->udid, budget, err);
    if (!dev->lockdown_) {
        err = "Failed to establish lockdown session: " + err;
        budget.check(err);
        return std::nullopt;
    }
    stage(verbose, "Start CoreDeviceProxy");
    auto ep = dev->lockdown_->start_service_bounded(kCoreDeviceProxy, budget, err);
    if (!ep) {
        err = "Start " + std::string(kCoreDeviceProxy) + " failed: " + err + proxy_failure_hint(err);
        budget.check(err);
        return std::nullopt;
    }
    stage(verbose, "CDTunnel handshake");
    auto tunnel =
        transport::PacketTunnel::establish_bounded(chosen->device_id, ep->port, dev->lockdown_->identity(), ep->requires_tls, budget, err);
    if (!tunnel) {
        err = "Failed to establish packet tunnel: " + err;
        budget.check(err);
        return std::nullopt;
    }
    // 从配对 HostID 确定性生成稳定 RSD 身份；已有 UUID 保持原值，
    // 非 UUID 标识经同一规则转换，不能为每次连接随机分配 peer UUID。
    auto uuid = peer_uuid_from_host_id(dev->lockdown_->host_id());
    if (!uuid) {
        err = "Cannot construct a stable peer identity from pairing HostID";
        budget.check(err);
        return std::nullopt;
    }
    PeerIdentity identity;
    identity.uuid = *uuid;

    stage(verbose, "Connect to RSD inside tunnel and read directory");
    if (!dev->finish_session(std::move(*tunnel), identity, budget, verbose, err)) {
        budget.check(err);
        return std::nullopt;
    }
    return dev;
}

bool Device::finish_session(
    transport::PacketTunnel&& tunnel,
    PeerIdentity identity,
    const transport::IoBudget& budget,
    bool verbose,
    std::string& err)
{
    tunnel_ = std::make_unique<transport::PacketTunnel>(std::move(tunnel));
    stage(verbose, "Connect to RSD inside tunnel and read directory");

    stack_ = std::make_unique<net::Stack>(*tunnel_, tunnel_->params().client_address, tunnel_->params().server_address);
    if (!stack_->addresses_ok()) {
        err = "Tunnel returned an invalid IPv6 address";
        return false;
    }
    // 隧道线程必须先于 TCP/RSD 连接启动，负责将收到的 IPv6 包交给协议栈。
    if (!stack_->start_pump(err)) {
        return false;
    }
    rsd_ = Rsd::open_bounded(*stack_, *tunnel_, identity, budget, err, verbose);
    if (!rsd_) {
        return false;
    }
    stage(verbose, "Ready");
    return true;
}

std::optional<Device>
    Device::establish_wifi(const std::string& address, const wifi::PairRecord& record, std::string& err, bool verbose, uint16_t port)
{
    transport::IoBudget budget;
    return establish_wifi_bounded(address, record, budget, err, verbose, port);
}

std::optional<Device> Device::establish_wifi_bounded(
    const std::string& address,
    const wifi::PairRecord& record,
    const transport::IoBudget& budget,
    std::string& err,
    bool verbose,
    uint16_t port)
{
    if (!budget.check(err)) {
        return std::nullopt;
    }
    // 本地记录问题应在连接前报出，避免网络不可达掩盖重新配对的原因。
    if (!record.complete()) {
        err = "Pairing record incomplete; cannot sign";
        budget.check(err);
        return std::nullopt;
    }
    if (!record.has_peer_identity()) {
        err = "Pairing record has no device identity; pair again over USB";
        budget.check(err);
        return std::nullopt;
    }
    std::optional<Device> dev;
    dev.emplace();
    dev->udid_ = record.udid;
    dev->connection_type_ = "WiFi";

    stage(verbose, "LAN pair-verify");
    // Preserve the short TCP fallback while all stages share the parent deadline.
    const auto connect = [&](uint16_t target_port) {
        transport::IoBudget tcp_budget(
            { std::min(budget.remaining(), std::chrono::milliseconds(5000)), [&budget] { return budget.stopped(); } });
        return transport::connect_tcp_bounded(address, target_port, tcp_budget, err);
    };
    auto control = connect(port);
    if (!control) {
        err = "Cannot connect to device RemotePairing port " + address + ":" + std::to_string(port) + "：" + err
              + " (check that the device is reachable on this network and has been paired)";
        budget.check(err);
        return std::nullopt;
    }
    transport::SocketBudgetGuard control_guard(*control, budget);
    wifi::SocketStream stream(*control);
    wifi::FramedCarrier carrier(stream);
    wifi::Rppairing channel(carrier);
    const wifi::PairVerifyResult verified = wifi::pair_verify(channel, record, err);
    if (verified.outcome != wifi::VerifyOutcome::Paired) {
        if (verified.outcome == wifi::VerifyOutcome::NotPaired) {
            err = "Device rejected pairing record: " + mask(record.udid, 8) + " is not paired with this device, or its pairing was removed";
        }
        else {
            err = "pair-verify failed: " + verified.error;
        }
        budget.check(err);
        return std::nullopt;
    }

    stage(verbose, "Request device tunnel port");
    const auto listener = wifi::request_tcp_listener(channel, verified.shared_secret, err);
    if (!listener) {
        budget.check(err);
        return std::nullopt;
    }
    // 已取得隧道监听端口，关闭配对控制通道；隧道使用独立 TCP 连接。
    if (!control_guard.finish(err)) {
        return std::nullopt;
    }
    control->close();

    stage(verbose, "TLS-PSK and CDTunnel handshake");
    auto tunnel_sock = connect(*listener);
    if (!tunnel_sock) {
        err = "Cannot connect to tunnel port " + std::to_string(*listener) + "：" + err;
        budget.check(err);
        return std::nullopt;
    }
    auto tunnel = transport::PacketTunnel::establish_psk_bounded(std::move(*tunnel_sock), verified.shared_secret, budget, err);
    if (!tunnel) {
        err = "Failed to establish packet tunnel: " + err;
        budget.check(err);
        return std::nullopt;
    }

    // 使用配对记录中持久化的 host identifier，保持多次会话的 RSD 身份一致。
    const auto uuid = parse_uuid_text(record.host_identifier);
    if (!uuid) {
        err = "Pairing host identifier is not a valid UUID; cannot construct a stable peer identity";
        budget.check(err);
        return std::nullopt;
    }
    PeerIdentity identity;
    identity.uuid = *uuid;
    if (!dev->finish_session(std::move(*tunnel), identity, budget, verbose, err)) {
        budget.check(err);
        return std::nullopt;
    }
    return dev;
}

void Device::interrupt_startup()
{
    if (tunnel_) {
        tunnel_->shutdown();
    }
}

std::string Device::property(std::string_view key) const
{
    const auto* props = rsd_ ? rsd_->properties() : nullptr;
    const auto* v = props != nullptr ? props->find(key) : nullptr;
    return v != nullptr ? v->as_string_or("") : std::string();
}

std::unique_ptr<ServiceConnection> Device::connect(std::string_view service_name, std::string& err, bool verbose, std::stop_token cancel)
{
    if (cancel.stop_requested()) {
        err = "Service connection cancelled";
        return nullptr;
    }
    if (!rsd_) {
        err = "Session not established";
        return nullptr;
    }
    return rsd_->connect_service(service_name, err, verbose, cancel);
}

CallResult Device::feature_call(
    std::string_view service_name,
    std::string_view feature_identifier,
    std::string_view action_identifier,
    const xpc::Value& input,
    xpc::Value& output,
    std::string& err,
    bool verbose,
    int timeout_ms)
{
    if (rsd_ && !rsd_->supports(service_name, feature_identifier)) {
        err = std::string(service_name) + " does not advertise feature " + std::string(feature_identifier);
        return CallResult::DeviceError;
    }
    auto conn = connect(service_name, err, verbose);
    if (conn == nullptr) {
        return CallResult::TransportError;
    }
    return conn->invoke(feature_identifier, action_identifier, input, output, timeout_ms, err);
}

bool Device::feature(
    std::string_view service_name,
    std::string_view feature_identifier,
    std::string_view action_identifier,
    const xpc::Value& input,
    xpc::Value& output,
    std::string& err,
    bool verbose,
    int timeout_ms)
{
    return feature_call(service_name, feature_identifier, action_identifier, input, output, err, verbose, timeout_ms) == CallResult::Ok;
}

} // namespace maa::ios::remote
