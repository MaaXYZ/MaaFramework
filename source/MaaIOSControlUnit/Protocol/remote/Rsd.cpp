#include "remote/Rsd.h"
#include "i18n/Translation.h"

#include <charconv>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace maa::ios::remote
{
namespace
{

// Channel has per-operation defaults; the watcher also closes the TCP endpoint
// at the shared absolute deadline, including while SETTINGS/peer_info trickle in.
// Destroy/join before the Rsd is moved or destroyed: the worker only borrows tcp.
class TcpBudgetGuard
{
public:
    TcpBudgetGuard(net::TcpStream& tcp, const transport::IoBudget& budget)
        : tcp_(tcp)
        , budget_(budget)
    {
        if (budget_.stopped()) {
            tcp_.close();
        }
        else {
            worker_ = std::thread([this] {
                std::unique_lock lock(mutex_);
                while (!done_) {
                    cv_.wait_for(lock, std::min(budget_.remaining(), std::chrono::milliseconds(25)), [this] { return done_; });
                    if (done_) {
                        break;
                    }
                    lock.unlock();
                    const bool stopped = budget_.stopped();
                    if (stopped) {
                        tcp_.close();
                    }
                    lock.lock();
                    if (stopped) {
                        break;
                    }
                }
            });
        }
    }

    ~TcpBudgetGuard() { stop(); }

    bool finish(std::string& err)
    {
        stop();
        return budget_.check(err);
    }

private:
    void stop()
    {
        {
            std::lock_guard lock(mutex_);
            done_ = true;
        }
        cv_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    net::TcpStream& tcp_;
    const transport::IoBudget& budget_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool done_ = false;
    std::thread worker_;
};

/// 当前 CoreDevice 请求使用的 DDI 协议版本和默认客户端版本声明。
constexpr int64_t kDdiProtocolVersion = 2;
constexpr const char* kCoreDeviceVersionString = "629.3";

uint16_t to_port(const xpc::Value* v)
{
    if (v == nullptr) {
        return 0;
    }
    if (v->is_string()) {
        // 只接受完整的十进制端口，不能将尾随内容或越界值截断为另一个服务的端口。
        uint64_t parsed = 0;
        const char* end = v->string.data() + v->string.size();
        const auto result = std::from_chars(v->string.data(), end, parsed, 10);
        if (result.ec == std::errc { } && result.ptr == end && parsed <= 65535) {
            return static_cast<uint16_t>(parsed);
        }
        return 0;
    }
    if (v->type == xpc::Type::Int64 && v->int64 >= 0 && v->int64 <= 65535) {
        return static_cast<uint16_t>(v->int64);
    }
    if (v->type == xpc::Type::UInt64 && v->uint64 <= 65535) {
        return static_cast<uint16_t>(v->uint64);
    }
    return 0;
}

std::string uuid_text_from(std::mt19937_64& rng)
{
    static constexpr char kHex[] = "0123456789abcdef";
    std::string s;
    for (int i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            s += '-';
            continue;
        }
        const unsigned v = i == 14 ? 4 : (i == 19 ? ((rng() & 0x3) | 0x8) : (rng() & 0xF));
        s += kHex[v & 0xF];
    }
    return s;
}

} // namespace

std::vector<ServiceInfo> parse_service_directory(const xpc::Value& services)
{
    std::vector<ServiceInfo> parsed;
    if (!services.is_dict()) {
        return parsed;
    }
    for (const auto& entry : services.dict) {
        if (!entry.value.is_dict()) {
            // 跳过非字典条目，保留其它可识别的服务。
            continue;
        }
        ServiceInfo si;
        si.name = entry.key;
        si.port = to_port(entry.value.find("Port"));
        si.entitlement = entry.value.at("Entitlement").as_string_or("");
        const auto& props = entry.value.at("Properties");
        if (props.is_dict()) {
            si.uses_remote_xpc = props.at("UsesRemoteXPC").as_bool_or(false);
            si.encrypt_socket_data = props.at("EncryptSocketData").as_bool_or(false);
            const auto& features = props.at("Features");
            for (const auto& f : features.array) {
                si.features.push_back(f.as_string_or(""));
            }
        }
        // 端口缺失、为 0 或无效时仍保留目录项；连接前另行检查，能力与诊断信息不丢失。
        parsed.push_back(std::move(si));
    }
    return parsed;
}

std::string random_uuid_text()
{
    // 追踪号用的随机源，不是密钥；用 random_device 播种即可。
    static thread_local std::mt19937_64 rng(
        std::random_device { }() ^ static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
    return uuid_text_from(rng);
}

xpc::Value core_device_request(std::string_view feature_identifier, std::string_view action_identifier, const xpc::Value& input)
{
    auto d = xpc::make_dict();
    xpc::dict_set(d, "CoreDevice.CoreDeviceDDIProtocolVersion", xpc::make_int64(kDdiProtocolVersion));

    auto version = xpc::make_dict();
    auto components = xpc::make_array();
    // 默认版本组件为 629.3。MAA_IOS_COREDEVICE_VERSION 用于调试客户端版本声明，
    // 按点或空格分隔数字组件；非法数字组件会保留默认 components。
    std::vector<uint64_t> parts { 629, 3 };
    const char* override = std::getenv("MAA_IOS_COREDEVICE_VERSION");
    if (override != nullptr && override[0] != '\0') {
        std::vector<uint64_t> parsed;
        std::string_view sv(override);
        std::size_t pos = 0;
        while (pos < sv.size()) {
            const auto sep = sv.find_first_of(". ", pos);
            // 最后一个组件取到字符串末尾，不包含分隔符。
            const auto token = sep == std::string_view::npos ? sv.substr(pos) : sv.substr(pos, sep - pos);
            bool bad = token.empty();
            uint64_t value = 0;
            for (char c : token) {
                if (c < '0' || c > '9') {
                    bad = true;
                    break;
                }
                value = value * 10 + static_cast<uint64_t>(c - '0');
            }
            if (bad) {
                std::fprintf(stderr, MAA_IOS_TR("Invalid numeric component in MAA_IOS_COREDEVICE_VERSION: %s (using 629.3)\n"), override);
                parsed.clear();
                break;
            }
            parsed.push_back(value);
            if (sep == std::string_view::npos) {
                break;
            }
            pos = sep + 1;
        }
        if (!parsed.empty()) {
            parts = std::move(parsed);
            std::fprintf(stderr, MAA_IOS_TR("  coreDeviceVersion overridden by environment: %s (default: 629.3)\n"), override);
        }
    }
    for (const uint64_t part : parts) {
        xpc::array_push(components, xpc::make_uint64(part));
    }
    xpc::dict_set(version, "components", std::move(components));
    xpc::dict_set(version, "originalComponentsCount", xpc::make_int64(static_cast<int64_t>(parts.size())));
    // 文本声明使用环境变量原值，并将空格分隔符规范化为点。
    std::string version_text = kCoreDeviceVersionString;
    if (override != nullptr && override[0] != '\0') {
        version_text = override;
        for (auto& c : version_text) {
            if (c == ' ') {
                c = '.';
            }
        }
    }
    xpc::dict_set(version, "stringValue", xpc::make_string(version_text));
    xpc::dict_set(d, "CoreDevice.coreDeviceVersion", std::move(version));

    // deviceIdentifier 默认每次生成 UUID，可用 MAA_IOS_DEVICE_IDENTIFIER 指定固定值。
    // invocationIdentifier 始终为本次调用生成新的追踪号。
    const char* pinned = std::getenv("MAA_IOS_DEVICE_IDENTIFIER");
    const std::string device_id = pinned != nullptr && pinned[0] != '\0' ? std::string(pinned) : random_uuid_text();
    xpc::dict_set(d, "CoreDevice.deviceIdentifier", xpc::make_string(device_id));
    xpc::dict_set(d, "CoreDevice.input", input);
    xpc::dict_set(d, "CoreDevice.invocationIdentifier", xpc::make_string(random_uuid_text()));
    if (!feature_identifier.empty()) {
        xpc::dict_set(d, "CoreDevice.featureIdentifier", xpc::make_string(std::string(feature_identifier)));
        // action 使用空字典，动作标识通过 actionIdentifier 单独发送。
        xpc::dict_set(d, "CoreDevice.action", xpc::make_dict());
    }
    if (!action_identifier.empty()) {
        xpc::dict_set(d, "CoreDevice.actionIdentifier", xpc::make_string(std::string(action_identifier)));
    }
    return d;
}

// 服务连接。

std::unique_ptr<ServiceConnection>
    ServiceConnection::open(net::Stack& stack, const ServiceInfo& service, std::string& err, bool verbose, std::stop_token cancel)
{
    if (cancel.stop_requested()) {
        err = MAA_IOS_TR("Service connection cancelled");
        return nullptr;
    }
    if (service.port == 0) {
        err = std::string(MAA_IOS_TR("Service has no valid TCP port (expected 1..65535): ")) + service.name;
        return nullptr;
    }
    auto conn = std::unique_ptr<ServiceConnection>(new ServiceConnection());
    conn->cancel_ = cancel;
    conn->tcp_ = std::make_unique<net::TcpStream>(stack);
    conn->cancellation_.emplace(cancel, CloseOnStop { conn->tcp_.get() });
    if (conn->cancelled(err)) {
        return nullptr;
    }
    if (!conn->tcp_->connect(service.port, err)) {
        if (conn->cancelled(err)) {
            return nullptr;
        }
        err =
            MAA_IOS_TR("Connect to ") + service.name + MAA_IOS_TR(" port ") + std::to_string(service.port) + MAA_IOS_TR(" failed: ") + err;
        return nullptr;
    }
    if (conn->cancelled(err)) {
        return nullptr;
    }
    if (!service.uses_remote_xpc) {
        // 非 RemoteXPC 服务仅建立 TCP，后续协议由 tcp() 的调用方处理。
        return conn;
    }
    auto channel = Channel::open(*conn->tcp_, err, verbose);
    if (conn->cancelled(err)) {
        return nullptr;
    }
    if (!channel) {
        err = service.name + MAA_IOS_TR(" HTTP/2 handshake failed: ") + err;
        return nullptr;
    }
    // 身份申报仅用于 RSD 控制通道；服务连接完成握手后直接发送业务请求。
    conn->channel_ = std::make_unique<Channel>(std::move(*channel));
    if (conn->cancelled(err)) {
        return nullptr;
    }
    return conn;
}

bool ServiceConnection::cancelled(std::string& err) const
{
    if (!cancel_.stop_requested()) {
        return false;
    }
    err = MAA_IOS_TR("Service connection cancelled");
    return true;
}

bool ServiceConnection::call(const xpc::Value& request, xpc::Value& reply, int timeout_ms, std::string& err)
{
    if (cancelled(err)) {
        return false;
    }
    if (channel_ == nullptr) {
        err = MAA_IOS_TR("Service connection is not RemoteXPC");
        return false;
    }
    const bool ok = channel_->call(request, reply, timeout_ms, err);
    return cancelled(err) ? false : ok;
}

bool ServiceConnection::send_only(const xpc::Value& request, std::string& err)
{
    if (cancelled(err)) {
        return false;
    }
    if (channel_ == nullptr) {
        err = MAA_IOS_TR("Service connection is not RemoteXPC");
        return false;
    }
    const bool ok = channel_->send_request(request, false, err);
    return cancelled(err) ? false : ok;
}

bool ServiceConnection::service(int timeout_ms, std::string& err)
{
    if (cancelled(err)) {
        return false;
    }
    if (channel_ == nullptr) {
        err = MAA_IOS_TR("Service connection is not RemoteXPC");
        return false;
    }
    const bool ok = channel_->service(timeout_ms, err);
    return cancelled(err) ? false : ok;
}

Channel::Wait ServiceConnection::wait_message(xpc::Value& out, int timeout_ms, std::string& err)
{
    if (cancelled(err)) {
        return Channel::Wait::Broken;
    }
    if (channel_ == nullptr) {
        err = MAA_IOS_TR("Service connection is not RemoteXPC");
        return Channel::Wait::Broken;
    }
    const auto status = channel_->wait(out, timeout_ms, err);
    return cancelled(err) ? Channel::Wait::Broken : status;
}

namespace
{

/// 从 CoreDevice.error 提取 code 和 userInfo 的描述、解码诊断及路径。
std::string device_error_text(std::string_view feature_identifier, const xpc::Value& error)
{
    const std::string detail = error.at("userInfo").at("NSLocalizedDescription").as_string_or("");
    const auto code = error.at("code").as_int_or(0);
    // 已收到业务错误，保留设备的说明和错误码，不在此自动重试。
    std::string err = std::string(feature_identifier) + MAA_IOS_TR(" failed");
    if (!detail.empty()) {
        err += "：" + detail;
    }
    err += MAA_IOS_TR("(code ") + std::to_string(code) + MAA_IOS_TR(")");
    err += Rsd::remote_control_version_hint(detail);
    // NSDebugDescription 保留原文，NSCodingPath 单独格式化，便于定位字段/类型错误。
    const std::string debug = error.at("userInfo").at("NSDebugDescription").as_string_or("");
    if (!debug.empty()) {
        err += "\n  NSDebugDescription: " + debug;
        // NSCodingPath 按 XPC 值描述，不能当作字符串读取。
        err += "\n  NSCodingPath: " + xpc::describe(error.at("userInfo").at("NSCodingPath"));
    }
    if (detail.empty() && debug.empty()) {
        // 没有文本诊断时附带截取后的原始错误对象。
        err += MAA_IOS_TR("; original error: ") + xpc::describe(error).substr(0, 600);
    }
    return err;
}

} // namespace

CallResult ServiceConnection::invoke(
    std::string_view feature_identifier,
    std::string_view action_identifier,
    const xpc::Value& input,
    xpc::Value& output,
    int timeout_ms,
    std::string& err)
{
    const auto request = core_device_request(feature_identifier, action_identifier, input);
    xpc::Value reply;
    if (!call(request, reply, timeout_ms, err)) {
        // 未取得有效回复，只能报告传输失败，不能判断设备是否执行了请求。
        return CallResult::TransportError;
    }
    const auto* out = reply.find("CoreDevice.output");
    if (out != nullptr) {
        output = *out;
        return CallResult::Ok;
    }
    output = xpc::make_dict();
    const auto* error = reply.find("CoreDevice.error");
    if (error == nullptr) {
        err = std::string(feature_identifier) + MAA_IOS_TR(" failed; response missing CoreDevice.output: ")
              + xpc::describe(reply).substr(0, 300);
        return CallResult::DeviceError;
    }
    err = device_error_text(feature_identifier, *error);
    return CallResult::DeviceError;
}

bool ServiceConnection::subscribe(
    std::string_view feature_identifier,
    std::string_view action_identifier,
    const xpc::Value& input,
    std::string& err)
{
    if (cancelled(err)) {
        return false;
    }
    if (channel_ == nullptr) {
        err = MAA_IOS_TR("Streaming feature requires an XPC service connection");
        return false;
    }
    if (!channel_->send_request(core_device_request(feature_identifier, action_identifier, input), true, err)) {
        cancelled(err);
        return false;
    }
    if (cancelled(err)) {
        return false;
    }
    subscribed_feature_ = std::string(feature_identifier);
    return true;
}

ServiceConnection::StreamEvent ServiceConnection::next_batch(std::vector<xpc::Value>& elements, int timeout_ms, std::string& err)
{
    elements.clear();
    if (channel_ == nullptr) {
        err = MAA_IOS_TR("Service connection is not an XPC channel");
        return StreamEvent::Broken;
    }
    xpc::Value reply;
    const auto w = wait_message(reply, timeout_ms, err);
    if (w == Channel::Wait::Timeout) {
        return StreamEvent::Idle;
    }
    if (w == Channel::Wait::Broken) {
        return StreamEvent::Broken;
    }
    const auto* status = reply.find("CoreDevice.XPCMessageKey.sideChannelStatus");
    if (status == nullptr) {
        // 订阅也可能通过普通 CoreDevice.error 返回失败，不附带 sideChannelStatus。
        const auto* error = reply.find("CoreDevice.error");
        if (error != nullptr) {
            err = device_error_text(subscribed_feature_, *error);
        }
        else {
            err = subscribed_feature_ + MAA_IOS_TR(" response missing both sideChannelStatus and error: ")
                  + xpc::describe(reply).substr(0, 300);
        }
        return StreamEvent::DeviceError;
    }
    if (status->find("receivedError") != nullptr) {
        err = subscribed_feature_ + MAA_IOS_TR(" failed mid-stream: ") + xpc::describe(status->at("receivedError")).substr(0, 400);
        return StreamEvent::DeviceError;
    }
    if (status->find("finishStreaming") != nullptr) {
        return StreamEvent::Finished;
    }
    const auto* pushing = status->find("pushing");
    const auto* batch = pushing == nullptr ? nullptr : pushing->find("elements");
    if (batch != nullptr) {
        for (const auto& element : batch->array) {
            elements.push_back(element);
        }
    }
    return StreamEvent::Batch;
}

CallResult ServiceConnection::stream(
    std::string_view feature_identifier,
    std::string_view action_identifier,
    const xpc::Value& input,
    const std::function<bool(const xpc::Value&)>& on_element,
    int timeout_ms,
    std::string& err)
{
    if (!subscribe(feature_identifier, action_identifier, input, err)) {
        return CallResult::TransportError;
    }
    for (;;) {
        std::vector<xpc::Value> batch;
        // 一次性 stream 将等待超时和断连都作为 TransportError；常驻订阅
        // 应使用 next_batch 区分 Idle，以便继续等待下一次状态推送。
        switch (next_batch(batch, timeout_ms, err)) {
        case StreamEvent::Idle:
        case StreamEvent::Broken:
            return CallResult::TransportError;
        case StreamEvent::DeviceError:
            return CallResult::DeviceError;
        case StreamEvent::Finished:
            return CallResult::Ok;
        case StreamEvent::Batch:
            break;
        }
        for (const auto& element : batch) {
            if (!on_element(element)) {
                return CallResult::Ok; // 回调要求结束本地消费。
            }
        }
    }
}

// RSD 目录。

std::optional<Rsd>
    Rsd::open(net::Stack& stack, transport::PacketTunnel& tunnel, const PeerIdentity& identity, std::string& err, bool verbose)
{
    const transport::IoBudget budget;
    return open_bounded(stack, tunnel, identity, budget, err, verbose);
}

std::optional<Rsd> Rsd::open_bounded(
    net::Stack& stack,
    transport::PacketTunnel& tunnel,
    const PeerIdentity& identity,
    const transport::IoBudget& budget,
    std::string& err,
    bool verbose)
{
    if (!budget.check(err)) {
        return std::nullopt;
    }
    const auto& p = tunnel.params();
    std::optional<Rsd> rsd;
    rsd.emplace(stack, identity);

    // Rsd 持有 TcpStream 和借用它的 Channel，按成员声明逆序先释放 control_。
    rsd->tcp_ = std::make_unique<net::TcpStream>(stack);
    TcpBudgetGuard guard(*rsd->tcp_, budget);
    if (!rsd->tcp_->connect(p.rsd_port, err)) {
        if (!budget.check(err)) {
            return std::nullopt;
        }
        err = MAA_IOS_TR("Connect to RSD port ") + std::to_string(p.rsd_port) + MAA_IOS_TR(" failed: ") + err;
        return std::nullopt;
    }
    if (!budget.check(err)) {
        return std::nullopt;
    }
    auto channel = Channel::open(*rsd->tcp_, err, verbose);
    if (!budget.check(err)) {
        return std::nullopt;
    }
    if (!channel) {
        err = MAA_IOS_TR("RSD control channel handshake failed: ") + err;
        return std::nullopt;
    }
    rsd->control_ = std::make_unique<Channel>(std::move(*channel));
    if (!rsd->control_->announce_device(identity, err)) {
        budget.check(err);
        return std::nullopt;
    }
    if (!budget.check(err)) {
        return std::nullopt;
    }
    const auto* info = rsd->control_->peer_info();
    const auto* services = info != nullptr ? info->find("Services") : nullptr;
    if (services == nullptr || !services->is_dict()) {
        err = MAA_IOS_TR("peer_info missing Services");
        return std::nullopt;
    }
    rsd->services_ = parse_service_directory(*services);
    if (!guard.finish(err)) {
        return std::nullopt;
    }
    return rsd;
}

std::optional<ServiceInfo> Rsd::service(std::string_view name) const
{
    for (const auto& s : services_) {
        if (s.name == name) {
            return s;
        }
    }
    return std::nullopt;
}

bool Rsd::has_service(std::string_view name) const
{
    return service(name).has_value();
}

const xpc::Value* Rsd::service_entry(std::string_view name) const
{
    const auto* info = control_ != nullptr ? control_->peer_info() : nullptr;
    const auto* services = info != nullptr ? info->find("Services") : nullptr;
    return services != nullptr ? services->find(name) : nullptr;
}

bool Rsd::supports(std::string_view service_name, std::string_view feature) const
{
    const auto info = service(service_name);
    if (!info) {
        return false;
    }
    for (const auto& f : info->features) {
        if (f == feature) {
            return true;
        }
    }
    return false;
}

std::unique_ptr<ServiceConnection> Rsd::connect_service(std::string_view name, std::string& err, bool verbose, std::stop_token cancel)
{
    if (cancel.stop_requested()) {
        err = MAA_IOS_TR("Service connection cancelled");
        return nullptr;
    }
    const auto info = service(name);
    if (!info) {
        err = missing_service_message(name, services_);
        return nullptr;
    }
    return ServiceConnection::open(*stack_, *info, err, verbose, cancel);
}

std::string Rsd::missing_service_message(const std::string_view name, const std::vector<ServiceInfo>& seen)
{
    // 报告目录总数，仅逐行列出 CoreDevice 服务名，其它服务不展开。
    // 目录能力只是诊断信息，不能单独证明 DDI 挂载状态；全量目录由 feature_probe 输出。
    std::string out = MAA_IOS_TR("Device directory missing service ") + std::string(name) + MAA_IOS_TR(".");
    if (seen.empty()) {
        out += MAA_IOS_TR("Directory is empty; check tunnel setup and DDI mounting.");
        out += MAA_IOS_TR(
            "\nDownload a supported DDI with MaaIOSDevice prepare; installation and compatibility guidance: the MaaIOSDevice "
            "documentation.\n");
        return out;
    }
    std::vector<std::string> cd;
    for (const auto& s : seen) {
        if (s.name.rfind("com.apple.coredevice", 0) == 0) {
            cd.push_back(s.name);
        }
    }
    out += MAA_IOS_TR("Device directory contains ") + std::to_string(seen.size()) + MAA_IOS_TR(" services, including ")
           + std::to_string(cd.size()) + MAA_IOS_TR(" com.apple.coredevice.* services:\n");
    if (cd.empty()) {
        out += MAA_IOS_TR(
            "  (none found; check DDI mounting through Xcode and the CoreDevice services "
            "available on this device)\n");
    }
    for (const auto& n : cd) {
        out += "  · " + n + "\n";
    }
    out += MAA_IOS_TR(
        "A mounted DDI may still lack this service. Check its version and capabilities; download a supported DDI with MaaIOSDevice "
        "prepare. Installation guidance: the MaaIOSDevice documentation.\n");
    out += MAA_IOS_TR(
               "Include this diagnostic when reporting the issue. For the full directory and "
               "feature lists, run feature_probe --all to print all ")
           + std::to_string(seen.size()) + MAA_IOS_TR(" services.\n");
    return out;
}

std::string Rsd::remote_control_version_hint(const std::string_view device_detail)
{
    // 根据设备错误的 requires iOS 文本添加提示，不把观测到的错误码作为版本判断。
    if (device_detail.find("requires iOS") == std::string_view::npos) {
        return "";
    }
    return MAA_IOS_TR(
        " (device rejected the system version: live media worked on tested iOS 27 "
        "devices and was rejected on tested iOS 18 devices. Screenshot fallback and "
        "button input may still be available; see docs/coredevice.md section 23)");
}

const xpc::Value* Rsd::properties() const
{
    const auto* info = control_ != nullptr ? control_->peer_info() : nullptr;
    return info != nullptr ? info->find("Properties") : nullptr;
}

} // namespace maa::ios::remote
