#pragma once

#include "tls_psk_move.h"
#include "transport/IoBudget.h"
#include "transport/Tunnel.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <thread>

namespace maa_ios_test
{

inline bool startup_tunnel_peer(maa::ios::transport::Socket& socket, bool drip)
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
    auto read_exact = [&](void* data, size_t len) {
        size_t got = 0;
        while (got < len) {
            int n = SSL_read(ssl.get(), static_cast<char*>(data) + got, static_cast<int>(len - got));
            if (n <= 0) {
                return false;
            }
            got += static_cast<size_t>(n);
        }
        return true;
    };
    unsigned char header[10] { };
    if (!read_exact(header, sizeof(header)) || std::memcmp(header, "CDTunnel", 8)) {
        return false;
    }
    size_t size = size_t(header[8]) * 256 + header[9];
    if (size > 1024) {
        return false;
    }
    std::string request(size, '\0');
    if (!read_exact(request.data(), size)) {
        return false;
    }
    const std::string body = R"({"clientParameters":{"address":"fd00::1","mtu":16000},"serverAddress":"fd00::2","serverRSDPort":1234})";
    std::string reply("CDTunnel", 8);
    reply.push_back(static_cast<char>(body.size() >> 8));
    reply.push_back(static_cast<char>(body.size()));
    reply += body;
    if (drip) {
        for (const auto byte : reply) {
            if (SSL_write(ssl.get(), &byte, 1) != 1) {
                return true; // Expected cancellation.
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        return false; // A full trickle reply must not beat the deadline.
    }
    if (SSL_write(ssl.get(), reply.data(), static_cast<int>(reply.size())) != static_cast<int>(reply.size())) {
        return false;
    }
    unsigned char packet[40] { };
    return read_exact(packet, sizeof(packet)) && packet[0] == 0x60;
}

// Real OpenSSL handshakes against local sockets: no phone or TLS mocks.
template <typename Check>
void startup_tls_budget(Check check)
{
    using namespace maa::ios::transport;
    using namespace std::chrono_literals;
    const std::vector<uint8_t> psk(move_psk.begin(), move_psk.end());
    const auto identity = failure_identity();
    for (const bool certificate : { false, true }) {
        Socket client, server;
        std::string err;
        if (!make_test_socket_pair(client, server, err)) {
            check(false, err.c_str());
            continue;
        }
        IoBudget budget(StartupOptions { 100ms, { } });
        TlsChannel tls;
        const auto start = std::chrono::steady_clock::now();
        const bool ok =
            certificate ? tls.handshake_bounded(client, identity, budget, err) : tls.handshake_psk_bounded(client, psk, budget, err);
        check(
            !ok && !tls.valid() && err.find("timed out") != std::string::npos && std::chrono::steady_clock::now() - start < 1500ms,
            certificate ? "silent certificate TLS peer respects total startup deadline"
                        : "silent PSK TLS peer respects total startup deadline");
    }
    {
        Socket client, server;
        std::string err;
        if (!make_test_socket_pair(client, server, err)) {
            check(false, err.c_str());
            return;
        }
        std::atomic<bool> cancelled { false };
        IoBudget budget(StartupOptions { 5s, [&] { return cancelled.load(); } });
        auto cancel = std::async(std::launch::async, [&] {
            std::this_thread::sleep_for(80ms);
            cancelled = true;
        });
        TlsChannel tls;
        const auto start = std::chrono::steady_clock::now();
        const bool ok = tls.handshake_psk_bounded(client, psk, budget, err);
        cancel.get();
        check(
            !ok && !tls.valid() && err.find("cancelled") != std::string::npos && std::chrono::steady_clock::now() - start < 1500ms,
            "cancellation interrupts a silent OpenSSL handshake");
    }
    {
        Socket client, server;
        std::string err;
        if (!make_test_socket_pair(client, server, err)) {
            check(false, err.c_str());
            return;
        }
        IoBudget budget(StartupOptions { 100ms, { } });
        auto drip = std::async(std::launch::async, [&] {
            const unsigned char prefix[] { 0x16, 0x03, 0x03, 0x00, 0x20 };
            for (const auto byte : prefix) {
                std::string ignored;
                if (!server.write_all(&byte, 1, ignored)) {
                    break;
                }
                std::this_thread::sleep_for(40ms);
            }
        });
        TlsChannel tls;
        const auto start = std::chrono::steady_clock::now();
        const bool ok = tls.handshake_psk_bounded(client, psk, budget, err);
        drip.get();
        check(
            !ok && !tls.valid() && err.find("timed out") != std::string::npos && std::chrono::steady_clock::now() - start < 1500ms,
            "TLS trickle traffic cannot reset the absolute deadline");
    }
    {
        Socket client, server;
        std::string err;
        if (!make_test_socket_pair(client, server, err)) {
            check(false, err.c_str());
            return;
        }
        // Once a successful handshake returns, its watcher must be joined.
        // Expiring the former startup budget must not kill normal traffic.
        IoBudget budget(StartupOptions { 500ms, { } });
        server.set_read_timeout(2500, err);
        auto peer = std::async(std::launch::async, [&] { return accept_psk(server); });
        TlsChannel tls;
        const bool ok = tls.handshake_psk_bounded(client, psk, budget, err);
        std::this_thread::sleep_for(600ms);
        const bool exchanged = ok && exchange(tls.handle());
        client.interrupt();
        check(ok && exchanged && peer.get(), "successful TLS handshake stops its startup watcher before normal I/O");
    }
    for (const bool drip : { true, false }) {
        Socket client, server;
        std::string err;
        if (!make_test_socket_pair(client, server, err)) {
            check(false, err.c_str());
            return;
        }
        server.set_read_timeout(2500, err);
        auto peer = std::async(std::launch::async, [&] { return startup_tunnel_peer(server, drip); });
        IoBudget budget(StartupOptions { drip ? 150ms : 500ms, { } });
        const auto start = std::chrono::steady_clock::now();
        auto tunnel = PacketTunnel::establish_psk_bounded(std::move(client), psk, budget, err);
        if (drip) {
            check(
                !tunnel && err.find("timed out") != std::string::npos && std::chrono::steady_clock::now() - start < 1500ms && peer.get(),
                "authenticated CDTunnel trickle reply shares the TLS startup deadline");
        }
        else {
            std::this_thread::sleep_for(600ms);
            unsigned char packet[40] { };
            packet[0] = 0x60;
            const bool sent = tunnel && tunnel->send_ipv6(packet, sizeof(packet), err);
            if (tunnel) {
                tunnel->shutdown();
            }
            else {
                server.interrupt();
            }
            check(sent && peer.get(), "successful CDTunnel joins its watcher before moving into the session");
        }
    }
    {
        Socket client, server;
        std::string err;
        if (!make_test_socket_pair(client, server, err)) {
            check(false, err.c_str());
            return;
        }
        IoBudget expired(StartupOptions { 0ms, { } });
        auto tunnel = PacketTunnel::establish_psk_bounded(std::move(client), psk, expired, err);
        check(
            !tunnel && client.valid() && err.find("timed out") != std::string::npos,
            "expired tunnel budget is rejected before consuming its socket");
    }
}

} // namespace maa_ios_test
