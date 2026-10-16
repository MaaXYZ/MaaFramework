#include "Usbmux.h"
#include "i18n/Translation.h"

#include "transport/TcpConnect.h"
#ifndef _WIN32
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstring>

namespace maa::ios::transport
{
namespace
{

constexpr uint32_t kProtoVersion = 1;
constexpr uint32_t kMsgPlist = 8;
constexpr size_t kHeaderLen = 16;
/// 一帧负载的上限。对方声明的长度必须先跟它比再去申请内存。
constexpr uint32_t kMaxPayload = 16u << 20;
constexpr uint16_t kLockdownPort = 62078;
constexpr const char* kClientName = "MaaFramework/iOS";

void put_u32_le(uint8_t* p, uint32_t v)
{
    p[0] = static_cast<uint8_t>(v);
    p[1] = static_cast<uint8_t>(v >> 8);
    p[2] = static_cast<uint8_t>(v >> 16);
    p[3] = static_cast<uint8_t>(v >> 24);
}

uint32_t get_u32_le(const uint8_t* p)
{
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}

plist::Value base_request(std::string_view type)
{
    plist::Value v = plist::Value::Dict();
    v.set("MessageType", plist::Value::Str(std::string(type)));
    v.set("ClientVersionString", plist::Value::Str(kClientName));
    v.set("ProgName", plist::Value::Str(kClientName));
    return v;
}

DeviceRecord parse_record(const plist::Value& rec)
{
    DeviceRecord d;
    if (const auto* id = rec.find("DeviceID")) {
        d.device_id = static_cast<uint32_t>(id->as_int_or(0));
    }
    if (const auto* props = rec.find("Properties")) {
        if (const auto* s = props->find("SerialNumber")) {
            d.udid = s->as_string_or("");
        }
        if (const auto* c = props->find("ConnectionType")) {
            d.connection_type = c->as_string_or("");
        }
        if (const auto* pid = props->find("ProductID")) {
            d.product_id = static_cast<uint32_t>(pid->as_int_or(0));
        }
    }
    return d;
}

// 独立枚举连接始终非阻塞，不把该状态/预算交给配对或媒体连接。
// 每次进展、EINTR 和 readiness 都只消耗同一个 deadline，不能重新计时。
class DiscoveryBudget
{
public:
    DiscoveryBudget(std::chrono::milliseconds timeout, const std::function<bool()>& cancel)
        : deadline_(std::chrono::steady_clock::now() + std::clamp(timeout, std::chrono::milliseconds(0), std::chrono::milliseconds(60000)))
        , cancel_(cancel)
    {
    }

    bool active()
    {
        if (status != UsbmuxDiscoveryStatus::complete) {
            return false;
        }
        if (cancel_ && cancel_()) {
            status = UsbmuxDiscoveryStatus::cancelled;
        }
        else if (std::chrono::steady_clock::now() >= deadline_) {
            status = UsbmuxDiscoveryStatus::timed_out;
        }
        return status == UsbmuxDiscoveryStatus::complete;
    }

    bool wait(NativeSocket fd, bool writing, std::string& err)
    {
        while (active()) {
            const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(deadline_ - std::chrono::steady_clock::now()).count();
            const int ready = wait_socket(fd, writing, static_cast<int>(std::clamp<int64_t>(remaining, 0, 50)));
            const int code = ready < 0 ? socket_error() : 0;
            if (!active()) {
                return false;
            }
            if (ready > 0) {
                return true;
            }
            if (ready < 0 && code != kSocketInterrupted) {
                err = socket_error_message(code);
                status = UsbmuxDiscoveryStatus::unavailable;
                return false;
            }
        }
        return false;
    }

    bool transfer(NativeSocket fd, void* data, size_t length, bool writing, std::string& err)
    {
        auto* bytes = static_cast<char*>(data);
        size_t offset = 0;
        while (offset < length && active()) {
            const auto count = static_cast<int>(std::min(length - offset, size_t(INT_MAX)));
#ifdef MSG_NOSIGNAL
            constexpr int send_flags = MSG_NOSIGNAL;
#else
            constexpr int send_flags = 0;
#endif
            const auto n = writing ? ::send(fd, bytes + offset, count, send_flags) : ::recv(fd, bytes + offset, count, 0);
            if (n > 0) {
                offset += static_cast<size_t>(n);
                continue;
            }
            if (n < 0) {
                const int code = socket_error();
                if (code == kSocketInterrupted) {
                    continue;
                }
                if (socket_read_timed_out(code)) {
                    if (!wait(fd, writing, err)) {
                        return false;
                    }
                    continue;
                }
                err = std::string(writing ? MAA_IOS_TR("send failed: ") : MAA_IOS_TR("recv failed: ")) + socket_error_message(code);
            }
            else {
                err = writing ? MAA_IOS_TR("Peer closed write direction")
                              : MAA_IOS_TR("Peer closed (received only ") + std::to_string(offset) + "/" + std::to_string(length)
                                    + MAA_IOS_TR(")");
            }
            status = UsbmuxDiscoveryStatus::unavailable;
            return false;
        }
        return active();
    }

    UsbmuxDiscoveryStatus status = UsbmuxDiscoveryStatus::complete;

private:
    std::chrono::steady_clock::time_point deadline_;
    const std::function<bool()>& cancel_;
};

} // namespace

UsbmuxDiscoveryStatus detail::list_usbmux_devices_at(
    const UsbmuxDiscoveryEndpoint& endpoint,
    std::vector<DeviceRecord>& out,
    std::chrono::milliseconds timeout,
    const std::function<bool()>& should_cancel,
    std::string& err)
{
    out.clear();
    err.clear();
    DiscoveryBudget budget(timeout, should_cancel);
    if (!budget.active()) {
        return budget.status;
    }
    if (!initialize_sockets(err)) {
        return UsbmuxDiscoveryStatus::unavailable;
    }
#ifdef _WIN32
    sockaddr_in address { };
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(endpoint.port);
    Socket socket(::socket(AF_INET, SOCK_STREAM, 0));
#else
    sockaddr_un address { };
    address.sun_family = AF_UNIX;
    if (endpoint.path.size() >= sizeof(address.sun_path)) {
        return UsbmuxDiscoveryStatus::unavailable;
    }
    std::memcpy(address.sun_path, endpoint.path.c_str(), endpoint.path.size() + 1);
    Socket socket(::socket(AF_UNIX, SOCK_STREAM, 0));
#endif
    if (!socket.valid()) {
        err = std::string(MAA_IOS_TR("socket failed: ")) + socket_error_message();
        return UsbmuxDiscoveryStatus::unavailable;
    }
    const NativeSocket fd = socket.fd();
#ifdef _WIN32
    u_long mode = 1;
    if (::ioctlsocket(fd, FIONBIO, &mode) != 0) {
#else
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
#endif
        err = MAA_IOS_TR("Failed to enable nonblocking mode: ") + socket_error_message();
        return UsbmuxDiscoveryStatus::unavailable;
    }
    if (!budget.active()) {
        return budget.status;
    }
    if (::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        const int code = socket_error();
        // AF_UNIX 的 backlog 满在部分系统返回 EAGAIN（未发起连接），直接失败；
        // 不能把它当成已连接，也不能退回阻塞 connect。
        if (!socket_connect_pending(code) && code != kSocketInterrupted) {
            err = socket_error_message(code);
            return UsbmuxDiscoveryStatus::unavailable;
        }
        if (!budget.wait(fd, true, err)) {
            return budget.status;
        }
        int so_error = 0;
        socklen_t size = sizeof(so_error);
        if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&so_error), &size) != 0 || so_error) {
            err = socket_error_message(so_error == 0 ? socket_error() : so_error);
            return UsbmuxDiscoveryStatus::unavailable;
        }
    }
    if (!budget.active()) {
        return budget.status;
    }
    std::string body = plist::write(base_request("ListDevices"));
    body.push_back('\0');
    uint8_t header[kHeaderLen];
    put_u32_le(header, static_cast<uint32_t>(kHeaderLen + body.size()));
    put_u32_le(header + 4, kProtoVersion);
    put_u32_le(header + 8, kMsgPlist);
    put_u32_le(header + 12, 1);
    if (!budget.transfer(fd, header, sizeof(header), true, err) || !budget.transfer(fd, body.data(), body.size(), true, err)
        || !budget.transfer(fd, header, sizeof(header), false, err)) {
        return budget.status;
    }
    const uint32_t total = get_u32_le(header);
    if (total < kHeaderLen || total - kHeaderLen > kMaxPayload) {
        err = MAA_IOS_TR("Invalid mux frame length: ") + std::to_string(total);
        return UsbmuxDiscoveryStatus::unavailable;
    }
    std::vector<uint8_t> payload(total - kHeaderLen);
    if (!budget.transfer(fd, payload.data(), payload.size(), false, err)) {
        return budget.status;
    }
    while (!payload.empty() && payload.back() == 0) {
        payload.pop_back();
    }
    auto reply = plist::parse(std::string_view(reinterpret_cast<const char*>(payload.data()), payload.size()));
    if (!budget.active()) {
        return budget.status;
    }
    if (!reply) {
        err = MAA_IOS_TR("mux response is not a valid plist");
        return UsbmuxDiscoveryStatus::unavailable;
    }
    const auto* list = reply->find("DeviceList");
    if (!list) {
        err = MAA_IOS_TR("ListDevices response missing DeviceList");
        return UsbmuxDiscoveryStatus::unavailable;
    }
    std::vector<DeviceRecord> records;
    for (const auto& record : list->array) {
        if (!budget.active()) {
            return budget.status;
        }
        records.push_back(parse_record(record));
    }
    if (!budget.active()) {
        return budget.status;
    }
    out = std::move(records);
    return UsbmuxDiscoveryStatus::complete;
}

UsbmuxDiscoveryStatus Usbmux::list_devices_for_discovery(
    std::vector<DeviceRecord>& out,
    std::chrono::milliseconds timeout,
    const std::function<bool()>& should_cancel,
    std::string& err)
{
    return detail::list_usbmux_devices_at({ }, out, timeout, should_cancel, err);
}

// ------------------------------------------------------------ Usbmux ------

std::string Usbmux::socket_path()
{
#ifdef _WIN32
    return "127.0.0.1:27015";
#else
    return "/var/run/usbmuxd";
#endif
}

std::optional<Usbmux> Usbmux::open(std::string& err)
{
#ifdef _WIN32
    // Apple Mobile Device Service 在 Windows 提供本地 TCP usbmux 服务。
    auto socket = connect_tcp("127.0.0.1", 27015, 3000, err);
    if (!socket) {
        return std::nullopt;
    }
    Usbmux mux;
    mux.sock_ = std::move(*socket);
    return mux;
#else
    const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        err = std::string(MAA_IOS_TR("socket failed: ")) + std::strerror(errno);
        return std::nullopt;
    }
    sockaddr_un addr { };
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socket_path().c_str(), sizeof(addr.sun_path) - 1);
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        err = MAA_IOS_TR("Connect to ") + socket_path() + MAA_IOS_TR(" failed: ") + std::strerror(errno);
        ::close(fd);
        return std::nullopt;
    }
    Usbmux mux;
    mux.sock_.reset(fd);
    return mux;
#endif
}

std::optional<Usbmux> Usbmux::open_for_service(std::chrono::milliseconds timeout, const std::function<bool()>& cancelled, std::string& err)
{
    err.clear();
    DiscoveryBudget budget(timeout, cancelled);
    auto fail = [&]() -> std::optional<Usbmux> {
        if (budget.status == UsbmuxDiscoveryStatus::cancelled) {
            err = MAA_IOS_TR("DDI service connection was cancelled");
        }
        else if (budget.status == UsbmuxDiscoveryStatus::timed_out) {
            err = MAA_IOS_TR("DDI service connection timed out");
        }
        return { };
    };
    if (!budget.active() || !initialize_sockets(err)) {
        return fail();
    }
#ifdef _WIN32
    sockaddr_in address { };
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(27015);
    Socket socket(::socket(AF_INET, SOCK_STREAM, 0));
    u_long mode = 1;
    if (!socket.valid() || ::ioctlsocket(socket.fd(), FIONBIO, &mode) != 0) {
#else
    sockaddr_un address { };
    address.sun_family = AF_UNIX;
    std::strncpy(address.sun_path, socket_path().c_str(), sizeof(address.sun_path) - 1);
    Socket socket(::socket(AF_UNIX, SOCK_STREAM, 0));
    const int flags = socket.valid() ? ::fcntl(socket.fd(), F_GETFL, 0) : -1;
    if (flags < 0 || ::fcntl(socket.fd(), F_SETFL, flags | O_NONBLOCK) != 0) {
#endif
        err = MAA_IOS_TR("Cannot create nonblocking DDI usbmux connection");
        return fail();
    }
    const auto fd = socket.fd();
    if (::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        const auto code = socket_error();
        if (!socket_connect_pending(code) && code != kSocketInterrupted) {
            err = socket_error_message(code);
            return fail();
        }
        if (!budget.wait(fd, true, err)) {
            return fail();
        }
        int code2 = 0;
        socklen_t length = sizeof(code2);
        if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&code2), &length) != 0 || code2) {
            err = socket_error_message(code2 == 0 ? socket_error() : code2);
            return fail();
        }
    }
    if (!budget.active()) {
        return fail();
    }
#ifdef _WIN32
    mode = 0;
    if (::ioctlsocket(fd, FIONBIO, &mode) != 0) {
#else
    if (::fcntl(fd, F_SETFL, flags) != 0) {
#endif
        err = MAA_IOS_TR("Cannot restore blocking DDI usbmux connection");
        return fail();
    }
    Usbmux mux;
    mux.sock_ = std::move(socket);
    return mux;
}

bool Usbmux::round_trip_for_service(
    const plist::Value& req,
    plist::Value& reply,
    std::chrono::milliseconds timeout,
    const std::function<bool()>& cancelled,
    std::string& err)
{
    err.clear();
    DiscoveryBudget budget(timeout, cancelled);
    auto fail = [&]() -> bool {
        if (budget.status == UsbmuxDiscoveryStatus::cancelled) {
            err = MAA_IOS_TR("DDI service connection was cancelled");
        }
        else if (budget.status == UsbmuxDiscoveryStatus::timed_out) {
            err = MAA_IOS_TR("DDI service connection timed out");
        }
        sock_.close();
        return false;
    };
    if (!budget.active() || !sock_.valid()) {
        return fail();
    }
    const auto fd = sock_.fd();
#ifdef _WIN32
    u_long mode = 1;
    if (::ioctlsocket(fd, FIONBIO, &mode) != 0) {
#else
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
#endif
        err = MAA_IOS_TR("Cannot configure nonblocking DDI usbmux connection");
        return fail();
    }
    auto body = plist::write(req);
    body += '\0';
    uint8_t header[kHeaderLen];
    put_u32_le(header, static_cast<uint32_t>(kHeaderLen + body.size()));
    put_u32_le(header + 4, kProtoVersion);
    put_u32_le(header + 8, kMsgPlist);
    put_u32_le(header + 12, tag_++);
    if (!budget.transfer(fd, header, sizeof(header), true, err) || !budget.transfer(fd, body.data(), body.size(), true, err)
        || !budget.transfer(fd, header, sizeof(header), false, err)) {
        return fail();
    }
    const auto total = get_u32_le(header);
    if (total < kHeaderLen || total - kHeaderLen > kMaxPayload) {
        err = MAA_IOS_TR("Invalid mux frame length: ") + std::to_string(total);
        return fail();
    }
    std::vector<uint8_t> payload(total - kHeaderLen);
    if (!budget.transfer(fd, payload.data(), payload.size(), false, err)) {
        return fail();
    }
    while (!payload.empty() && payload.back() == 0) {
        payload.pop_back();
    }
    auto parsed = plist::parse(std::string_view(reinterpret_cast<const char*>(payload.data()), payload.size()));
    if (!parsed || !parsed->is_dict()) {
        err = MAA_IOS_TR("DDI usbmux response is not a plist dictionary");
        return fail();
    }
    reply = std::move(*parsed);
    if (!budget.active()) {
        return fail();
    }
#ifdef _WIN32
    mode = 0;
    if (::ioctlsocket(fd, FIONBIO, &mode) != 0) {
#else
    if (::fcntl(fd, F_SETFL, flags) != 0) {
#endif
        err = MAA_IOS_TR("Cannot restore blocking DDI usbmux connection");
        return fail();
    }
    return true;
}

std::optional<Socket> Usbmux::connect_for_service(
    uint32_t device_id,
    uint16_t port,
    std::chrono::milliseconds timeout,
    const std::function<bool()>& cancelled,
    std::string& err)
{
    if (!port) {
        err = MAA_IOS_TR("DDI usbmux service port is invalid");
        return { };
    }
    auto req = base_request("Connect");
    req.set("DeviceID", plist::Value::Int(device_id));
    req.set("PortNumber", plist::Value::Int(htons(port)));
    plist::Value reply;
    if (!round_trip_for_service(req, reply, timeout, cancelled, err)) {
        return { };
    }
    const auto* number = reply.find("Number");
    if (!number || !number->is_int() || number->integer != 0) {
        err = MAA_IOS_TR("DDI usbmux Connect was rejected");
        sock_.close();
        return { };
    }
    return Socket(sock_.release());
}

bool Usbmux::read_pair_record_for_service(
    std::string_view udid,
    std::vector<uint8_t>& out,
    std::chrono::milliseconds timeout,
    const std::function<bool()>& cancelled,
    std::string& err)
{
    auto req = base_request("ReadPairRecord");
    req.set("PairRecordID", plist::Value::Str(std::string(udid)));
    plist::Value reply;
    if (!round_trip_for_service(req, reply, timeout, cancelled, err)) {
        return false;
    }
    const auto* data = reply.find("PairRecordData");
    if (!data || data->kind != plist::Kind::Data || data->data.empty()) {
        err = MAA_IOS_TR("ReadPairRecord response missing PairRecordData");
        return false;
    }
    out = data->data;
    return true;
}

Usbmux::Usbmux(Usbmux&& other) noexcept
    : sock_(std::move(other.sock_))
    , tag_(other.tag_)
{
    other.tag_ = 0;
}

Usbmux& Usbmux::operator=(Usbmux&& other) noexcept
{
    if (this != &other) {
        sock_ = std::move(other.sock_);
        tag_ = other.tag_;
        other.tag_ = 0;
    }
    return *this;
}

bool Usbmux::round_trip(const plist::Value& request, plist::Value& reply, std::string& err)
{
    std::string body = plist::write(request);
    body.push_back('\0');

    uint8_t hdr[kHeaderLen];
    put_u32_le(hdr, static_cast<uint32_t>(kHeaderLen + body.size()));
    put_u32_le(hdr + 4, kProtoVersion);
    put_u32_le(hdr + 8, kMsgPlist);
    put_u32_le(hdr + 12, tag_);
    if (!sock_.write_all(hdr, kHeaderLen, err)) {
        return false;
    }
    if (!sock_.write_all(body.data(), body.size(), err)) {
        return false;
    }
    ++tag_;

    uint8_t rhdr[kHeaderLen];
    if (!sock_.read_exact(rhdr, kHeaderLen, err)) {
        return false;
    }
    const uint32_t total = get_u32_le(rhdr);
    // 上限必须有，且不能只判下界：total 是对方给的 32 位数，只挡 `< kHeaderLen`
    // 的话，一个 0xFFFFFFFF 就会让我们先去申请 4GB 内存——那是 DoS，不是解析失败。
    // usbmuxd 的回复实际是 KB 级（设备列表、配对记录），16MB 已经宽到没边。
    if (total < kHeaderLen || total - kHeaderLen > kMaxPayload) {
        err = MAA_IOS_TR("Invalid mux frame length: ") + std::to_string(total);
        return false;
    }
    std::vector<uint8_t> payload(total - kHeaderLen);
    if (!payload.empty() && !sock_.read_exact(payload.data(), payload.size(), err)) {
        return false;
    }
    while (!payload.empty() && payload.back() == 0) {
        payload.pop_back();
    }
    auto parsed = plist::parse(std::string_view(reinterpret_cast<const char*>(payload.data()), payload.size()));
    if (!parsed) {
        err = MAA_IOS_TR("mux response is not a valid plist");
        return false;
    }
    reply = std::move(*parsed);
    return true;
}

bool Usbmux::list_devices(std::vector<DeviceRecord>& out, std::string& err)
{
    plist::Value reply;
    if (!round_trip(base_request("ListDevices"), reply, err)) {
        return false;
    }
    const auto* list = reply.find("DeviceList");
    if (list == nullptr) {
        err = MAA_IOS_TR("ListDevices response missing DeviceList");
        return false;
    }
    for (const auto& rec : list->array) {
        out.push_back(parse_record(rec));
    }
    return true;
}

bool Usbmux::list_devices_for_service(
    std::vector<DeviceRecord>& out,
    std::chrono::milliseconds timeout,
    const std::function<bool()>& cancelled,
    std::string& err)
{
    out.clear();
    plist::Value reply;
    if (!round_trip_for_service(base_request("ListDevices"), reply, timeout, cancelled, err)) {
        return false;
    }
    const auto* list = reply.find("DeviceList");
    if (!list || list->kind != plist::Kind::Array) {
        err = MAA_IOS_TR("ListDevices response missing DeviceList");
        return false;
    }
    for (const auto& rec : list->array) {
        out.push_back(parse_record(rec));
    }
    return true;
}

std::optional<Socket> Usbmux::connect(uint32_t device_id, uint16_t port, std::string& err)
{
    plist::Value req = base_request("Connect");
    req.set("DeviceID", plist::Value::Int(device_id));
    // 必须网络序：传主机序会得到 Number=3 (CONNREFUSED)。
    req.set("PortNumber", plist::Value::Int(htons(port)));

    plist::Value reply;
    if (!round_trip(req, reply, err)) {
        return std::nullopt;
    }
    const auto* number_val = reply.find("Number");
    const int number = number_val != nullptr ? static_cast<int>(number_val->as_int_or(-1)) : -1;
    if (number != 0) {
        err = MAA_IOS_TR("Connect failed, usbmuxd returned Number=") + std::to_string(number);
        return std::nullopt;
    }
    // 此后同一 socket 即到 device:port 的透明通道，把 fd 交出去。
    Socket tunnel;
    tunnel.reset(sock_.release());
    return tunnel;
}

bool Usbmux::read_pair_record(std::string_view udid, std::vector<uint8_t>& out, std::string& err)
{
    plist::Value req = base_request("ReadPairRecord");
    req.set("PairRecordID", plist::Value::Str(std::string(udid)));

    plist::Value reply;
    if (!round_trip(req, reply, err)) {
        return false;
    }
    const auto* data = reply.find("PairRecordData");
    if (data == nullptr || data->data.empty()) {
        const auto* e = reply.find("MessageType");
        err = MAA_IOS_TR("ReadPairRecord response missing PairRecordData (response ") + std::string(e ? e->as_string_or("?") : "?")
              + MAA_IOS_TR("); device may not trust this computer yet");
        return false;
    }
    out = data->data;
    return true;
}

std::optional<Socket> connect_lockdown(uint32_t device_id, std::string& err)
{
    auto mux = Usbmux::open(err);
    if (!mux) {
        return std::nullopt;
    }
    return mux->connect(device_id, kLockdownPort, err);
}

} // namespace maa::ios::transport
