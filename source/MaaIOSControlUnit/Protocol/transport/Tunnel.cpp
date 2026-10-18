#include "Tunnel.h"

#include <cerrno>
#include <cstring>
#include <openssl/err.h>

#include "transport/SocketPlatform.h"
#include "json/Json.h"

namespace maa::ios::transport
{
namespace
{

constexpr std::string_view kMagic = "CDTunnel";
constexpr size_t kControlHeaderLen = 10;  // magic(8) + u16 长度
constexpr size_t kIpv6HeaderLen = 40;
constexpr uint16_t kRequestedMtu = 16000; // 与参考实现的 TCP 隧道一致

void put_be16(uint8_t* p, uint16_t v)
{
    p[0] = static_cast<uint8_t>(v >> 8);
    p[1] = static_cast<uint8_t>(v);
}

uint16_t get_be16(const uint8_t* p)
{
    return static_cast<uint16_t>(uint16_t(p[0]) << 8 | p[1]);
}

// SSL_get_error depends on the calling thread's error queue. Start each I/O
// with a clean queue and native error state, then capture both before formatting
// anything. In particular, a stale errno must not turn EOF into a socket error.
void clear_tls_io_errors()
{
    ERR_clear_error();
    errno = 0;
#ifdef _WIN32
    WSASetLastError(0);
#endif
}

std::string tls_io_failure(std::string_view operation, int returned, int ssl_error, int saved_errno, int saved_socket_error)
{
    const char* category = "UNKNOWN";
    switch (ssl_error) {
    case SSL_ERROR_NONE:
        category = "NONE (short write)";
        break;
    case SSL_ERROR_ZERO_RETURN:
        category = "ZERO_RETURN (close_notify)";
        break;
    case SSL_ERROR_SYSCALL:
        category = returned == 0 && saved_errno == 0 && saved_socket_error == 0 ? "SYSCALL (EOF without close_notify)" : "SYSCALL";
        break;
    case SSL_ERROR_SSL:
        category = "SSL";
        break;
    case SSL_ERROR_WANT_READ:
        category = "WANT_READ";
        break;
    case SSL_ERROR_WANT_WRITE:
        category = "WANT_WRITE";
        break;
    }
    std::string message(operation);
    message += ": return=" + std::to_string(returned) + ", ssl_error=" + std::to_string(ssl_error) + " " + category
               + ", errno=" + std::to_string(saved_errno);
#ifdef _WIN32
    message += ", socket_error=" + std::to_string(saved_socket_error);
#endif
    // Error codes/reasons contain no application bytes. Do not retrieve the
    // queue's optional extra data (which may contain protocol content).
    for (unsigned long code; (code = ERR_get_error()) != 0;) {
        char reason[256] { };
        ERR_error_string_n(code, reason, sizeof(reason));
        message += "; ";
        message += reason;
    }
    return message;
}

} // namespace

PacketTunnel::PacketTunnel(PacketTunnel&&) noexcept = default;
PacketTunnel& PacketTunnel::operator=(PacketTunnel&&) noexcept = default;

bool PacketTunnel::write_all(const void* data, size_t len, std::string& err)
{
    if (tls_.handle() != nullptr) {
        clear_tls_io_errors();
        const int n = SSL_write(tls_.handle(), data, static_cast<int>(len));
        const int saved_errno = errno;
        const int saved_socket_error = socket_error();
        if (n != static_cast<int>(len)) {
            const int ssl_error = SSL_get_error(tls_.handle(), n);
            return err = tls_io_failure("Tunnel TLS write failed", n, ssl_error, saved_errno, saved_socket_error), false;
        }
        return true;
    }
    return sock_.write_all(data, len, err);
}

bool PacketTunnel::read_all(void* data, size_t len, std::string& err)
{
    if (tls_.handle() != nullptr) {
        auto* p = static_cast<uint8_t*>(data);
        size_t got = 0;
        while (got < len) {
            clear_tls_io_errors();
            const int n = SSL_read(tls_.handle(), p + got, static_cast<int>(len - got));
            const int saved_errno = errno;
            const int saved_socket_error = socket_error();
            if (n <= 0) {
                const int ssl_error = SSL_get_error(tls_.handle(), n);
                return err = tls_io_failure("Tunnel TLS read failed", n, ssl_error, saved_errno, saved_socket_error), false;
            }
            got += static_cast<size_t>(n);
        }
        return true;
    }
    return sock_.read_exact(data, len, err);
}

std::optional<PacketTunnel>
    PacketTunnel::establish(uint32_t device_id, uint16_t proxy_port, const PemIdentity& identity, bool use_tls, std::string& err)
{
    auto mux = Usbmux::open(err);
    if (!mux) {
        return std::nullopt;
    }
    auto sock = mux->connect(device_id, proxy_port, err);
    if (!sock) {
        return std::nullopt;
    }

    PacketTunnel t;
    t.sock_ = std::move(*sock);
    if (use_tls && !t.tls_.handshake(t.sock_, identity, err)) {
        return std::nullopt;
    }
    if (!t.client_handshake(err)) {
        return std::nullopt;
    }
    return t;
}

std::optional<PacketTunnel> PacketTunnel::establish_psk(Socket&& sock, const std::vector<uint8_t>& psk, std::string& err)
{
    PacketTunnel t;
    t.sock_ = std::move(sock);
    // PSK 要在 CDTunnel 控制帧之前：明文发过去会被立刻关掉，和 lockdown 那条一样。
    if (!t.tls_.handshake_psk(t.sock_, psk, err)) {
        return std::nullopt;
    }
    if (!t.client_handshake(err)) {
        return std::nullopt;
    }
    return t;
}

std::optional<PacketTunnel> PacketTunnel::establish_bounded(
    uint32_t device_id,
    uint16_t proxy_port,
    const PemIdentity& identity,
    bool use_tls,
    const IoBudget& budget,
    std::string& err)
{
    if (!budget.check(err)) {
        return { };
    }
    auto cancelled = [&budget] {
        return budget.stopped();
    };
    auto mux = Usbmux::open_for_service(budget.remaining(), cancelled, err);
    if (!mux) {
        budget.check(err);
        return { };
    }
    auto socket = mux->connect_for_service(device_id, proxy_port, budget.remaining(), cancelled, err);
    if (!socket) {
        budget.check(err);
        return { };
    }
    PacketTunnel t;
    t.sock_ = std::move(*socket);
    if (use_tls && !t.tls_.handshake_bounded(t.sock_, identity, budget, err)) {
        return { };
    }
    SocketBudgetGuard guard(t.sock_, budget);
    const bool ok = budget.check(err) && t.client_handshake(err);
    if (!guard.finish(err) || !ok) {
        t.sock_.interrupt();
        return { };
    }
    return t;
}

std::optional<PacketTunnel>
    PacketTunnel::establish_psk_bounded(Socket&& socket, const std::vector<uint8_t>& psk, const IoBudget& budget, std::string& err)
{
    if (!budget.check(err)) {
        return { };
    }
    PacketTunnel t;
    t.sock_ = std::move(socket);
    if (!t.tls_.handshake_psk_bounded(t.sock_, psk, budget, err)) {
        return { };
    }
    SocketBudgetGuard guard(t.sock_, budget);
    const bool ok = budget.check(err) && t.client_handshake(err);
    if (!guard.finish(err) || !ok) {
        t.sock_.interrupt();
        return { };
    }
    return t;
}

bool PacketTunnel::client_handshake(std::string& err)
{
    const json::Value req = { { "type", "clientHandshakeRequest" }, { "mtu", kRequestedMtu } };
    const std::string body = json::write(req);

    std::vector<uint8_t> frame(kControlHeaderLen + body.size());
    std::memcpy(frame.data(), kMagic.data(), kMagic.size());
    put_be16(frame.data() + 8, static_cast<uint16_t>(body.size()));
    std::memcpy(frame.data() + kControlHeaderLen, body.data(), body.size());
    if (!write_all(frame.data(), frame.size(), err)) {
        return false;
    }

    uint8_t hdr[kControlHeaderLen];
    if (!read_all(hdr, kControlHeaderLen, err)) {
        return false;
    }
    if (std::memcmp(hdr, kMagic.data(), kMagic.size()) != 0) {
        err = "Tunnel handshake response has incorrect magic";
        return false;
    }
    const uint16_t payload_len = get_be16(hdr + 8);
    std::vector<uint8_t> payload(payload_len);
    if (!payload.empty() && !read_all(payload.data(), payload.size(), err)) {
        return false;
    }

    auto parsed = json::parse(std::string_view(reinterpret_cast<const char*>(payload.data()), payload.size()));
    if (!parsed) {
        err = "Tunnel handshake response is not valid JSON";
        return false;
    }
    const auto* cp = json::find(*parsed, "clientParameters");
    const auto* address = cp ? json::find(*cp, "address") : nullptr;
    const auto* server = json::find(*parsed, "serverAddress");
    const auto* port = json::find(*parsed, "serverRSDPort");
    const auto* mtu = cp ? json::find(*cp, "mtu") : nullptr;
    if (!mtu) {
        mtu = json::find(*parsed, "mtu");
    }
    params_.client_address = address ? json::as_string_or(*address) : "";
    params_.server_address = server ? json::as_string_or(*server) : "";
    params_.rsd_port = static_cast<uint16_t>(port ? json::as_int_or(*port) : 0);
    params_.mtu = static_cast<uint16_t>(mtu ? json::as_int_or(*mtu) : 0);

    if (params_.client_address.empty() || params_.server_address.empty() || params_.rsd_port == 0) {
        err = "Tunnel handshake response missing required fields";
        return false;
    }
    return true;
}

bool PacketTunnel::wait_readable(int ms, std::string& err, bool* timed_out)
{
    if (timed_out) {
        *timed_out = false;
    }
    // TLS 记录可能已被 SSL_read 拆进内部缓冲，此时 fd 上无可读事件，
    // 必须先问 SSL_pending，否则会白等超时。
    if (tls_.handle() != nullptr && SSL_pending(tls_.handle()) > 0) {
        return true;
    }
    return sock_.wait_readable(ms, err, timed_out);
}

bool PacketTunnel::send_ipv6(const uint8_t* packet, size_t len, std::string& err)
{
    if (!sock_.valid()) {
        err = "Tunnel not connected";
        return false;
    }
    // 一次一个包。合并写会破坏 CoreDeviceProxy 的读边界，导致隧道死亡。
    return write_all(packet, len, err);
}

bool PacketTunnel::recv_ipv6(std::vector<uint8_t>& out, std::string& err)
{
    if (!sock_.valid()) {
        err = "Tunnel not connected";
        return false;
    }
    uint8_t hdr[kIpv6HeaderLen];
    if (!read_all(hdr, kIpv6HeaderLen, err)) {
        return false;
    }
    if ((hdr[0] >> 4) != 6) {
        err = "Tunnel packet is not IPv6: version nibble=" + std::to_string(hdr[0] >> 4);
        return false;
    }
    const size_t total = kIpv6HeaderLen + get_be16(hdr + 4);
    out.resize(total);
    std::memcpy(out.data(), hdr, kIpv6HeaderLen);
    return read_all(out.data() + kIpv6HeaderLen, total - kIpv6HeaderLen, err);
}

} // namespace maa::ios::transport
