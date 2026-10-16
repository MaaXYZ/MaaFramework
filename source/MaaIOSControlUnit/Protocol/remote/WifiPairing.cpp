#include "remote/WifiPairing.h"

#include "i18n/Translation.h"
#include "transport/Socket.h"
#include "wifi/DiscoveryIdentity.h"
#include "wifi/Mdns.h"
#include "wifi/PairSetup.h"
#include "wifi/PairableHost.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <climits>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <memory>
#include <utility>
#ifndef _WIN32
#include <arpa/inet.h>
#include <fcntl.h>
#endif

namespace maa::ios::remote
{
namespace
{
using Clock = std::chrono::steady_clock;
using transport::Socket;

struct Deadline
{
    Clock::time_point until;
    const WifiPairingOptions& options;

    bool active(std::string& error) const
    {
        if (options.should_cancel && options.should_cancel()) {
            error = MAA_IOS_TR("Wi-Fi pairing cancelled");
            return false;
        }
        if (Clock::now() >= until) {
            error = MAA_IOS_TR("Wi-Fi pairing timed out");
            return false;
        }
        return true;
    }

    int remaining() const
    {
        return static_cast<int>(std::max<int64_t>(0, std::chrono::ceil<std::chrono::milliseconds>(until - Clock::now()).count()));
    }
};

bool nonblocking(Socket& socket, std::string& error)
{
#ifdef _WIN32
    u_long mode = 1;
    const bool ok = ::ioctlsocket(socket.fd(), FIONBIO, &mode) == 0;
#else
    const int flags = ::fcntl(socket.fd(), F_GETFL, 0);
    const bool ok = flags >= 0 && ::fcntl(socket.fd(), F_SETFL, flags | O_NONBLOCK) == 0;
#endif
    if (!ok) {
        error = MAA_IOS_TR("Cannot configure Wi-Fi pairing socket: ") + transport::socket_error_message();
    }
    return ok;
}

bool wait(
    Socket& socket,
    bool writing,
    const Deadline& deadline,
    std::string& error,
    const std::function<bool(std::string&)>& service = { })
{
    while (deadline.active(error)) {
        if (service && !service(error)) {
            return false;
        }
        const int status = transport::wait_socket(socket.fd(), writing, std::min(50, deadline.remaining()));
        if (status > 0) {
            return true;
        }
        if (status < 0 && transport::socket_error() != transport::kSocketInterrupted) {
            error = MAA_IOS_TR("Wi-Fi pairing socket wait failed: ") + transport::socket_error_message();
            return false;
        }
    }
    return false;
}

// Nonblocking exact I/O checks the total deadline and cancellation between all
// fragments. SO_RCVTIMEO alone would let a slow peer reset the timeout forever.
class PairingStream final : public wifi::ByteStream
{
public:
    PairingStream(Socket& socket, const Deadline& deadline, std::function<bool(std::string&)> service = { })
        : socket_(socket)
        , deadline_(deadline)
        , service_(std::move(service))
    {
    }

    bool write_all(const void* data, size_t size, std::string& error) override
    {
        return transfer(const_cast<void*>(data), size, true, error);
    }

    bool read_exact(void* data, size_t size, std::string& error) override { return transfer(data, size, false, error); }

    bool wait_readable(int ms, std::string& error) override
    {
        const Deadline shorter { std::min(deadline_.until, Clock::now() + std::chrono::milliseconds(std::max(0, ms))), deadline_.options };
        return wait(socket_, false, shorter, error, service_);
    }

private:
    bool transfer(void* data, size_t size, bool writing, std::string& error)
    {
        auto* bytes = static_cast<char*>(data);
        while (size && deadline_.active(error)) {
            if (!wait(socket_, writing, deadline_, error, service_)) {
                return false;
            }
            const int count = static_cast<int>(std::min<size_t>(size, INT_MAX));
            int flags = 0;
#ifdef MSG_NOSIGNAL
            if (writing) {
                flags = MSG_NOSIGNAL;
            }
#endif
            const auto done = writing ? ::send(socket_.fd(), bytes, count, flags) : ::recv(socket_.fd(), bytes, count, 0);
            if (done < 0) {
                const int code = transport::socket_error();
                if (code == transport::kSocketInterrupted || transport::socket_read_timed_out(code)) {
                    continue;
                }
                error = MAA_IOS_TR("Wi-Fi pairing socket I/O failed: ") + transport::socket_error_message(code);
                return false;
            }
            if (done == 0) {
                error = MAA_IOS_TR("Wi-Fi pairing connection closed");
                return false;
            }
            bytes += done;
            size -= static_cast<size_t>(done);
        }
        return size == 0;
    }

    Socket& socket_;
    const Deadline& deadline_;
    std::function<bool(std::string&)> service_;
};

std::optional<Socket> listen(int family, uint16_t& port, std::string& error)
{
    Socket socket(::socket(family, SOCK_STREAM, IPPROTO_TCP));
    if (!socket.valid()) {
        error = transport::socket_error_message();
        return std::nullopt;
    }
    sockaddr_storage address { };
    socklen_t length;
    if (family == AF_INET) {
        auto& a = reinterpret_cast<sockaddr_in&>(address);
        a.sin_family = AF_INET;
        a.sin_port = htons(port);
        a.sin_addr.s_addr = htonl(INADDR_ANY);
        length = sizeof(a);
    }
    else {
        const int one = 1;
        if (::setsockopt(socket.fd(), IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&one), sizeof(one)) != 0) {
            error = transport::socket_error_message();
            return std::nullopt;
        }
        auto& a = reinterpret_cast<sockaddr_in6&>(address);
        a.sin6_family = AF_INET6;
        a.sin6_port = htons(port);
        a.sin6_addr = in6addr_any;
        length = sizeof(a);
    }
    if (::bind(socket.fd(), reinterpret_cast<const sockaddr*>(&address), length) != 0 || ::listen(socket.fd(), 4) != 0
        || !nonblocking(socket, error) || ::getsockname(socket.fd(), reinterpret_cast<sockaddr*>(&address), &length) != 0) {
        if (error.empty()) {
            error = transport::socket_error_message();
        }
        return std::nullopt;
    }
    port = ntohs(family == AF_INET ? reinterpret_cast<sockaddr_in&>(address).sin_port : reinterpret_cast<sockaddr_in6&>(address).sin6_port);
    return socket;
}

std::optional<Socket> connect_numeric(const wifi::mdns::Endpoint& endpoint, const Deadline& deadline, std::string& error)
{
    if (!deadline.active(error)) {
        return std::nullopt;
    }
    addrinfo hints { };
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
    addrinfo* raw = nullptr;
    const auto service = std::to_string(endpoint.port);
    const int rc = ::getaddrinfo(endpoint.address.c_str(), service.c_str(), &hints, &raw);
    if (rc != 0 || !raw) {
        error = MAA_IOS_TR("Invalid numeric Wi-Fi pairing endpoint");
        return std::nullopt;
    }
    std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> addresses(raw, freeaddrinfo);
    for (auto* ai = raw; ai && deadline.active(error); ai = ai->ai_next) {
        Socket socket(::socket(ai->ai_family, SOCK_STREAM, IPPROTO_TCP));
        if (!socket.valid() || !nonblocking(socket, error)) {
            continue;
        }
        if (::connect(socket.fd(), ai->ai_addr, static_cast<socklen_t>(ai->ai_addrlen)) != 0) {
            if (!transport::socket_connect_pending(transport::socket_error())) {
                error = transport::socket_error_message();
                continue;
            }
            // A broken candidate must not consume the whole PIN-pairing deadline.
            const Deadline candidate { std::min(deadline.until, Clock::now() + std::chrono::seconds(3)), deadline.options };
            if (!wait(socket, true, candidate, error)) {
                continue;
            }
            int code = 0;
            socklen_t length = sizeof(code);
            if (::getsockopt(socket.fd(), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&code), &length) != 0 || code) {
                error = transport::socket_error_message(code ? code : transport::socket_error());
                continue;
            }
        }
        error.clear();
        return socket;
    }
    return std::nullopt;
}

std::string uuid(std::string& error)
{
    auto bytes = wifi::random_bytes(16, error);
    if (!bytes) {
        return { };
    }
    (*bytes)[6] = ((*bytes)[6] & 0x0f) | 0x40;
    (*bytes)[8] = ((*bytes)[8] & 0x3f) | 0x80;
    constexpr char hex[] = "0123456789ABCDEF";
    std::string result;
    for (size_t i = 0; i < bytes->size(); ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) {
            result += '-';
        }
        result += hex[(*bytes)[i] >> 4];
        result += hex[(*bytes)[i] & 15];
    }
    return result;
}

std::string pin(std::string& error)
{
    // Rejection sampling avoids modulo bias in a six-digit one-use PIN.
    uint32_t number;
    do {
        const auto bytes = wifi::random_bytes(4, error);
        if (!bytes) {
            return { };
        }
        number = uint32_t((*bytes)[0]) << 24 | uint32_t((*bytes)[1]) << 16 | uint32_t((*bytes)[2]) << 8 | (*bytes)[3];
    } while (number >= 4'294'000'000u);
    char text[7];
    std::snprintf(text, sizeof(text), "%06u", number % 1'000'000u);
    return text;
}

void progress(const WifiPairingOptions& options, std::string_view message)
{
    if (options.progress) {
        options.progress(message);
    }
}

bool record_text_valid(std::string_view value)
{
    return value.size() <= 255 && (value.empty() || (value.front() != ' ' && value.back() != ' '))
           && std::none_of(value.begin(), value.end(), [](unsigned char ch) { return ch < 0x20 || ch == 0x7f; });
}

} // namespace

bool detail::admit_wifi_pairing(const wifi::PairRecord& record, const WifiPairingOptions& options, std::string& err)
{
    err.clear();
    for (const auto& existing : options.existing_records) {
        if (existing.udid == record.udid) {
            err = "A credential for this device was supplied; verify it instead of pairing again";
            return false;
        }
    }
    // Only peer fields are available here: M5 has authenticated the Identifier
    // and public key, but the responder has not yet emitted its M6 identity.
    if (record.udid.empty() || !record_text_valid(record.udid) || !record.has_peer_identity() || record.peer_alt_irk.size() != 16
        || (!options.udid.empty() && options.udid != record.udid)) {
        err = MAA_IOS_TR("Wi-Fi pairing returned an incomplete or unexpected device identity");
        return false;
    }
    try {
        if (options.should_cancel && options.should_cancel()) {
            err = MAA_IOS_TR("Wi-Fi pairing cancelled");
            return false;
        }
        return true;
    }
    catch (const std::exception&) {
        err = MAA_IOS_TR("Wi-Fi pairing workflow failed before completion");
        return false;
    }
}

PairingResult detail::save_wifi_pairing(
    const wifi::PairRecord& record,
    const WifiPairingOptions& options,
    const std::function<wifi::PairVerifyResult(const wifi::PairRecord&, std::string&)>& verify)
{
    PairingResult result;
    result.udid = record.udid;
    if (!record.complete() || !record.has_peer_identity() || record.peer_alt_irk.size() != 16 || !record_text_valid(record.udid)
        || !record_text_valid(record.host_identifier) || !record_text_valid(record.advertised_identifier)
        || !record_text_valid(record.remote_unlock_host_key) || (!options.udid.empty() && options.udid != record.udid)) {
        result.error = MAA_IOS_TR("Wi-Fi pairing returned an incomplete or unexpected device identity");
        return result;
    }
    try {
        if (!admit_wifi_pairing(record, options, result.error)) {
            return result;
        }
        if (options.should_cancel && options.should_cancel()) {
            result.error = MAA_IOS_TR("Wi-Fi pairing cancelled");
            return result;
        }
        std::string error;
        const auto verified = verify(record, error);
        if (verified.outcome != wifi::VerifyOutcome::Paired) {
            result.error = MAA_IOS_TR("New Wi-Fi pairing record verification failed: ") + (verified.error.empty() ? error : verified.error);
            return result;
        }
        if (options.should_cancel && options.should_cancel()) {
            result.error = MAA_IOS_TR("Wi-Fi pairing cancelled");
            return result;
        }
        result.ok = true;
        result.record = record;
    }
    catch (const std::exception&) {
        result.error = MAA_IOS_TR("Wi-Fi pairing workflow failed before completion");
    }
    return result;
}

PairingResult pair_wifi_remote(const WifiPairingOptions& options)
{
    PairingResult result;
    if (options.timeout_ms < 1 || options.timeout_ms > 300000 || !options.display_pin) {
        result.error = MAA_IOS_TR("Wi-Fi pairing requires a PIN display callback and a timeout between 1 and 300000 ms");
        return result;
    }
    const Deadline deadline { Clock::now() + std::chrono::milliseconds(options.timeout_ms), options };
    try {
        if (!deadline.active(result.error) || !transport::initialize_sockets(result.error)) {
            return result;
        }
        wifi::PairableHostOptions host;
        host.host_identifier = uuid(result.error);
        host.host_name = wifi::local_hostname();
        const auto key = wifi::ed25519_keypair(result.error);
        const auto irk = wifi::random_bytes(16, result.error);
        host.setup_pin = pin(result.error);
        if (host.host_identifier.empty() || !key || !irk || host.setup_pin.empty()) {
            return result;
        }
        host.host_private_key.assign(key->seed.begin(), key->seed.end());
        host.host_public_key.assign(key->pub.begin(), key->pub.end());
        host.host_alt_irk = *irk;
        const auto hash = wifi::siphash24(wifi::sv(*irk), host.host_identifier, result.error);
        if (!hash) {
            return result;
        }
        wifi::Bytes tag(hash->begin(), hash->begin() + 6);
        std::reverse(tag.begin(), tag.end());

        std::vector<Socket> listeners;
        uint16_t port = 0;
        auto v4 = listen(AF_INET, port, result.error);
        if (v4) {
            listeners.push_back(std::move(*v4));
        }
        auto v6 = listen(AF_INET6, port, result.error);
        if (v6) {
            listeners.push_back(std::move(*v6));
        }
        if (listeners.empty()) {
            return result;
        }
        result.error.clear();
        wifi::mdns::HostAdvertisement advertisement { host.host_identifier, host.host_name, host.host_model, wifi::b64_encode(tag), port };
        advertisement.advertise_ipv4 = v4.has_value();
        advertisement.advertise_ipv6 = v6.has_value();
        auto advertised = wifi::mdns::Advertiser::start(advertisement, result.error);
        if (!advertised) {
            return result;
        }
        progress(
            options,
            MAA_IOS_TR("Open Settings > Privacy & Security > Developer Mode on the device and select Pair with this MaaIOS computer"));
        progress(options, host.host_name);
        Socket accepted;
        std::string peer_address;
        while (!accepted.valid() && deadline.active(result.error)) {
            if (!advertised->poll(std::min(50, deadline.remaining()), result.error)) {
                return result;
            }
            for (auto& listener : listeners) {
                sockaddr_storage peer { };
                socklen_t length = sizeof(peer);
                const auto fd = ::accept(listener.fd(), reinterpret_cast<sockaddr*>(&peer), &length);
                if (fd == transport::kInvalidSocket) {
                    const auto code = transport::socket_error();
                    if (code == transport::kSocketInterrupted || transport::socket_read_timed_out(code)) {
                        continue;
                    }
                    result.error = MAA_IOS_TR("Cannot accept Wi-Fi pairing connection: ") + transport::socket_error_message(code);
                    return result;
                }
                accepted.reset(fd);
                char numeric[NI_MAXHOST];
                if (::getnameinfo(reinterpret_cast<const sockaddr*>(&peer), length, numeric, sizeof(numeric), nullptr, 0, NI_NUMERICHOST)
                        != 0
                    || !nonblocking(accepted, result.error)) {
                    if (result.error.empty()) {
                        result.error = MAA_IOS_TR("Cannot identify the Wi-Fi pairing peer address");
                    }
                    return result;
                }
                peer_address = numeric;
                break;
            }
        }
        listeners.clear();
        if (!accepted.valid()) {
            return result;
        }
        // The phone's pairing UI continues tracking discovery during PIN entry.
        // Keep responding and refreshing until M6 instead of sending a goodbye
        // as soon as it opens the TCP connection.
        PairingStream stream(accepted, deadline, [&](std::string& error) { return advertised->poll(0, error); });
        wifi::FramedCarrier carrier(stream);
        const auto setup = wifi::accept_pairable_host(
            carrier,
            host,
            options.display_pin,
            result.error,
            [&](const wifi::PairableHostResult& peer, std::string& error) {
                // The peer UDID is now authenticated. Refuse duplicates and an
                // unexpected device before sending M6, not only before saving.
                return deadline.active(error) && detail::admit_wifi_pairing(peer.record, options, error) && deadline.active(error);
            });
        if (!setup || !deadline.active(result.error)) {
            return result;
        }
        accepted.close();
        advertised.reset();
        progress(options, MAA_IOS_TR("Reconnecting to verify the new Wi-Fi pairing record"));
        auto verify = [&](const wifi::PairRecord& record, std::string& error) {
            wifi::PairVerifyResult failed;
            std::vector<wifi::mdns::Endpoint> candidates;
            const wifi::mdns::BrowseOptions scan { std::chrono::milliseconds(std::min(3000, deadline.remaining())),
                                                   [&] { return !deadline.active(error); } };
            const auto found = wifi::mdns::browse(scan);
            if (!deadline.active(error)) {
                failed.error = error;
                return failed;
            }
            for (const auto& ad : found.advertisements) {
                if (wifi::match_advertisement(ad.identifier, ad.auth_tag, { record }).status != wifi::DiscoveryIdentityStatus::matched) {
                    continue;
                }
                candidates.insert(candidates.end(), ad.endpoints.begin(), ad.endpoints.end());
            }
            candidates.push_back({ peer_address, 49152, 0, { } });
            for (const auto& candidate : candidates) {
                if (!deadline.active(error)) {
                    break;
                }
                auto socket = connect_numeric(candidate, deadline, error);
                if (!socket) {
                    continue;
                }
                // A responsive but incorrect peer also has a finite per-candidate
                // budget. Never learn a replacement identity from its replies.
                const Deadline reply { std::min(deadline.until, Clock::now() + std::chrono::seconds(10)), options };
                PairingStream io(*socket, reply);
                wifi::FramedCarrier framing(io);
                wifi::Rppairing channel(framing);
                auto verified = wifi::pair_verify(channel, record, error, false);
                if (verified.outcome == wifi::VerifyOutcome::Paired) {
                    return verified;
                }
                failed = std::move(verified);
            }
            if (failed.error.empty()) {
                failed.error = error;
            }
            return failed;
        };
        WifiPairingOptions bounded_options = options;
        bounded_options.should_cancel = [&] {
            return !deadline.active(result.error);
        };
        auto saved = detail::save_wifi_pairing(setup->record, bounded_options, verify);
        if (!saved.ok && !deadline.active(result.error)) {
            saved.error = result.error;
        }
        return saved;
    }
    catch (const std::exception&) {
        result.error = MAA_IOS_TR("Wi-Fi pairing workflow failed before completion");
        return result;
    }
}
} // namespace maa::ios::remote
