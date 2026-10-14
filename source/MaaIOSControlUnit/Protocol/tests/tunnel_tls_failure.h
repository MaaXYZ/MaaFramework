#pragma once

#include "tls_psk_move.h"
#include "transport/Tunnel.h"

#include <cerrno>
#include <cstring>

namespace maa_ios_test
{

// Exercise PacketTunnel's actual TLS I/O through a local authenticated peer.
// Only the CDTunnel handshake is emulated; no diagnostic implementation is copied.
enum class TunnelPeerEnd
{
    CloseNotify,
    AbruptEof,
    WaitForLocalShutdown
};

inline bool tunnel_failure_peer(maa::ios::transport::Socket& socket, TunnelPeerEnd end, std::future<void>& release)
{
    auto context = std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)>(SSL_CTX_new(TLS_server_method()), SSL_CTX_free);
    if (!context || SSL_CTX_set_min_proto_version(context.get(), TLS1_2_VERSION) != 1
        || SSL_CTX_set_max_proto_version(context.get(), TLS1_2_VERSION) != 1
        || SSL_CTX_set_cipher_list(context.get(), "PSK-AES128-GCM-SHA256") != 1) {
        return false;
    }
    SSL_CTX_set_psk_server_callback(context.get(), move_server_psk);
    auto ssl = std::unique_ptr<SSL, decltype(&SSL_free)>(SSL_new(context.get()), SSL_free);
    if (!ssl || SSL_set_fd(ssl.get(), static_cast<int>(socket.fd())) != 1 || SSL_accept(ssl.get()) != 1) {
        return false;
    }
    const auto read_exact = [&](char* bytes, size_t length) {
        size_t got = 0;
        while (got < length) {
            const int n = SSL_read(ssl.get(), bytes + got, static_cast<int>(length - got));
            if (n <= 0) {
                return false;
            }
            got += static_cast<size_t>(n);
        }
        return true;
    };
    char header[10] { };
    if (!read_exact(header, sizeof(header)) || std::memcmp(header, "CDTunnel", 8) != 0) {
        return false;
    }
    const size_t size = static_cast<unsigned char>(header[8]) * 256u + static_cast<unsigned char>(header[9]);
    if (size > 1024) {
        return false;
    }
    std::string request(size, '\0');
    if (!read_exact(request.data(), request.size())) {
        return false;
    }
    const std::string body = R"({"clientParameters":{"address":"fd00::1","mtu":16000},"serverAddress":"fd00::2","serverRSDPort":1234})";
    std::string reply("CDTunnel", 8);
    reply.push_back(static_cast<char>(body.size() >> 8));
    reply.push_back(static_cast<char>(body.size()));
    reply += body;
    if (SSL_write(ssl.get(), reply.data(), static_cast<int>(reply.size())) != static_cast<int>(reply.size())) {
        return false;
    }
    if (end == TunnelPeerEnd::CloseNotify) {
        if (SSL_shutdown(ssl.get()) < 0) {
            return false;
        }
    }
    else if (end == TunnelPeerEnd::WaitForLocalShutdown) {
        release.wait();
    }
    ssl.reset(); // No implicit close_notify, so AbruptEof is a real bare socket EOF.
    socket.close();
    return true;
}

template <typename Check>
void tunnel_tls_failures(Check check)
{
    using namespace maa::ios::transport;
    for (const auto end : { TunnelPeerEnd::CloseNotify, TunnelPeerEnd::AbruptEof, TunnelPeerEnd::WaitForLocalShutdown }) {
        Socket client, server;
        std::string err;
        if (!make_test_socket_pair(client, server, err)) {
            check(false, err.c_str());
            continue;
        }
        client.set_read_timeout(2000, err);
        server.set_read_timeout(2000, err);
        std::promise<void> stop;
        auto release = stop.get_future();
        auto peer = std::async(std::launch::async, [&] { return tunnel_failure_peer(server, end, release); });
        auto tunnel = PacketTunnel::establish_psk(std::move(client), { move_psk.begin(), move_psk.end() }, err);
        check(tunnel.has_value(), "local PSK peer completes PacketTunnel handshake");
        if (!tunnel) {
            stop.set_value();
            server.interrupt();
            peer.get();
            continue;
        }
        if (end != TunnelPeerEnd::WaitForLocalShutdown) {
            check(peer.get(), "local peer closes after the tunnel handshake");
        }
        // Both native and OpenSSL errors are deliberately stale before the I/O.
        ERR_put_error(ERR_LIB_SSL, 0, SSL_R_CERTIFICATE_VERIFY_FAILED, __FILE__, __LINE__);
        errno = EINVAL;
#ifdef _WIN32
        WSASetLastError(WSAEINVAL);
#endif
        if (end == TunnelPeerEnd::WaitForLocalShutdown) {
            tunnel->shutdown();
            const uint8_t packet[40] { 0x60 };
            check(
                !tunnel->send_ipv6(packet, sizeof(packet), err) && err.find("Tunnel TLS write failed:") != std::string::npos
                    && err.find("SYSCALL") != std::string::npos,
                "interrupted tunnel write reports its native TLS failure");
            stop.set_value();
            check(peer.get(), "write fixture releases its peer");
        }
        else {
            std::vector<uint8_t> packet;
            const bool failed = !tunnel->recv_ipv6(packet, err);
            if (end == TunnelPeerEnd::CloseNotify) {
                check(
                    failed && err.find("ZERO_RETURN (close_notify)") != std::string::npos && err.find("errno=0") != std::string::npos,
                    "TLS close_notify is distinguished from native I/O failure");
            }
            else {
                // OpenSSL 1.1 reports SYSCALL/zero; OpenSSL 3 reports SSL with
                // unexpected-eof reason. Both describe an unannounced close.
                check(
                    failed
                        && (err.find("EOF without close_notify") != std::string::npos
                            || err.find("unexpected eof while reading") != std::string::npos),
                    "bare peer EOF is distinguished from TLS close_notify");
            }
        }
        check(
            err.find("certificate verify failed") == std::string::npos && ERR_peek_error() == 0,
            "tunnel I/O excludes stale OpenSSL errors and drains its own queue");
#ifdef _WIN32
        check(err.find("socket_error=") != std::string::npos, "Windows tunnel diagnostic includes captured socket error");
#endif
    }
}

} // namespace maa_ios_test
