#pragma once
#include "socket_pair.h"
#include "transport/TlsChannel.h"
#include <algorithm>
#include <array>
#include <future>
#include <memory>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/sslerr.h>

namespace maa_ios_test
{
inline constexpr std::array<unsigned char, 32> move_psk { 1, 2, 3, 4 };

inline unsigned move_server_psk(SSL*, const char* identity, unsigned char* out, unsigned capacity)
{
    if (!identity || *identity || capacity < move_psk.size()) {
        return 0;
    }
    std::copy(move_psk.begin(), move_psk.end(), out);
    return static_cast<unsigned>(move_psk.size());
}

inline bool accept_psk(maa::ios::transport::Socket& socket)
{
    auto context = std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)>(SSL_CTX_new(TLS_server_method()), SSL_CTX_free);
    if (!context || SSL_CTX_set_min_proto_version(context.get(), TLS1_2_VERSION) != 1
        || SSL_CTX_set_max_proto_version(context.get(), TLS1_2_VERSION) != 1
        || SSL_CTX_set_cipher_list(context.get(), "PSK-AES128-GCM-SHA256") != 1) {
        return false;
    }
    SSL_CTX_set_psk_server_callback(context.get(), move_server_psk);
    auto ssl = std::unique_ptr<SSL, decltype(&SSL_free)>(SSL_new(context.get()), SSL_free);
    char bytes[4] { };
    const bool connected = ssl && SSL_set_fd(ssl.get(), static_cast<int>(socket.fd())) == 1 && SSL_accept(ssl.get()) == 1;
    const bool ok =
        connected && SSL_read(ssl.get(), bytes, 4) == 4 && std::string(bytes, 4) == "ping" && SSL_write(ssl.get(), "pong", 4) == 4;
    if (connected) {
        SSL_shutdown(ssl.get());
    }
    return ok;
}

inline bool exchange(SSL* ssl)
{
    char reply[4] { };
    return SSL_write(ssl, "ping", 4) == 4 && SSL_read(ssl, reply, 4) == 4 && std::string(reply, 4) == "pong";
}

// 本地生成完整的 PEM 身份，分别破坏根证书、主机证书或私钥，覆盖各个解析阶段。
inline maa::ios::transport::PemIdentity failure_identity()
{
    auto generator =
        std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>(EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr), EVP_PKEY_CTX_free);
    EVP_PKEY* generated = nullptr;
    if (!generator || EVP_PKEY_keygen_init(generator.get()) != 1
        || EVP_PKEY_CTX_set_ec_paramgen_curve_nid(generator.get(), NID_X9_62_prime256v1) != 1
        || EVP_PKEY_keygen(generator.get(), &generated) != 1) {
        return { };
    }
    auto key = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>(generated, EVP_PKEY_free);
    auto cert = std::unique_ptr<X509, decltype(&X509_free)>(X509_new(), X509_free);
    if (!cert || X509_set_version(cert.get(), 2) != 1 || ASN1_INTEGER_set(X509_get_serialNumber(cert.get()), 1) != 1
        || !X509_gmtime_adj(X509_getm_notBefore(cert.get()), -60) || !X509_gmtime_adj(X509_getm_notAfter(cert.get()), 3600)
        || X509_set_pubkey(cert.get(), key.get()) != 1) {
        return { };
    }
    auto* name = X509_get_subject_name(cert.get());
    if (!name
        || X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, reinterpret_cast<const unsigned char*>("MaaIOS TLS test"), -1, -1, 0) != 1
        || X509_set_issuer_name(cert.get(), name) != 1 || X509_sign(cert.get(), key.get(), EVP_sha256()) <= 0) {
        return { };
    }
    auto cert_bio = std::unique_ptr<BIO, decltype(&BIO_free)>(BIO_new(BIO_s_mem()), BIO_free);
    auto key_bio = std::unique_ptr<BIO, decltype(&BIO_free)>(BIO_new(BIO_s_mem()), BIO_free);
    if (!cert_bio || !key_bio || PEM_write_bio_X509(cert_bio.get(), cert.get()) != 1
        || PEM_write_bio_PrivateKey(key_bio.get(), key.get(), nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        return { };
    }
    char *cert_bytes = nullptr, *key_bytes = nullptr;
    const auto cert_size = BIO_get_mem_data(cert_bio.get(), &cert_bytes);
    const auto key_size = BIO_get_mem_data(key_bio.get(), &key_bytes);
    if (cert_size <= 0 || key_size <= 0) {
        return { };
    }
    maa::ios::transport::PemIdentity identity;
    identity.host_cert.assign(cert_bytes, cert_bytes + cert_size);
    identity.host_key.assign(key_bytes, key_bytes + key_size);
    identity.root_cert = identity.host_cert;
    return identity;
}

inline bool accept_certificate(maa::ios::transport::Socket& socket, const maa::ios::transport::PemIdentity& identity)
{
    auto context = std::unique_ptr<SSL_CTX, decltype(&SSL_CTX_free)>(SSL_CTX_new(TLS_server_method()), SSL_CTX_free);
    auto cert_bio = std::unique_ptr<BIO, decltype(&BIO_free)>(
        BIO_new_mem_buf(identity.host_cert.data(), static_cast<int>(identity.host_cert.size())),
        BIO_free);
    auto key_bio = std::unique_ptr<BIO, decltype(&BIO_free)>(
        BIO_new_mem_buf(identity.host_key.data(), static_cast<int>(identity.host_key.size())),
        BIO_free);
    if (!context || !cert_bio || !key_bio) {
        return false;
    }
    auto cert = std::unique_ptr<X509, decltype(&X509_free)>(PEM_read_bio_X509(cert_bio.get(), nullptr, nullptr, nullptr), X509_free);
    auto key = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>(
        PEM_read_bio_PrivateKey(key_bio.get(), nullptr, nullptr, nullptr),
        EVP_PKEY_free);
    if (!cert || !key || SSL_CTX_use_certificate(context.get(), cert.get()) != 1 || SSL_CTX_use_PrivateKey(context.get(), key.get()) != 1) {
        return false;
    }
    auto ssl = std::unique_ptr<SSL, decltype(&SSL_free)>(SSL_new(context.get()), SSL_free);
    const bool connected = ssl && SSL_set_fd(ssl.get(), static_cast<int>(socket.fd())) == 1 && SSL_accept(ssl.get()) == 1;
    char bytes[4] { };
    const bool ok =
        connected && SSL_read(ssl.get(), bytes, 4) == 4 && std::string(bytes, 4) == "ping" && SSL_write(ssl.get(), "pong", 4) == 4;
    if (connected) {
        SSL_shutdown(ssl.get());
    }
    return ok;
}

// 仅通过公开 API 检查失败后的句柄与重试，不读取 TlsChannel 的内部所有权状态。
template <typename Check>
void tls_failures(Check check)
{
    using namespace maa::ios::transport;
    const std::vector<uint8_t> key { move_psk.begin(), move_psk.end() };
    const auto identity = failure_identity();
    if (!identity.complete()) {
        check(false, "create complete PEM fixture for failure paths");
        return;
    }
    TlsChannel channel;
    Socket invalid;
    std::string err;
    ERR_put_error(ERR_LIB_SSL, 0, SSL_R_CERTIFICATE_VERIFY_FAILED, __FILE__, __LINE__);
    check(
        !channel.handshake_psk(invalid, { }, err) && !err.empty() && !channel.valid() && channel.handle() == nullptr,
        "empty PSK fails without publishing a TLS handle");
    check(ERR_peek_error() == 0, "even an early handshake failure clears prior OpenSSL errors");
    check(
        !channel.handshake_psk(invalid, key, err) && !err.empty() && !channel.valid() && channel.handle() == nullptr,
        "invalid socket fails without publishing a TLS handle");

    // 原错误不能参与 SSL_get_error 或替代本次失败的原因。
    Socket plaintext_client, plaintext_server;
    if (!make_test_socket_pair(plaintext_client, plaintext_server, err)) {
        check(false, err.c_str());
        return;
    }
    plaintext_client.set_read_timeout(2000, err);
    plaintext_server.write_all("HTTP/1.1 400 Bad Request\r\n\r\n", 28, err);
    ERR_put_error(ERR_LIB_SSL, 0, SSL_R_CERTIFICATE_VERIFY_FAILED, __FILE__, __LINE__);
    check(
        !channel.handshake_psk(plaintext_client, key, err) && !channel.valid() && channel.handle() == nullptr
            && err.find("wrong version number") != std::string::npos && err.find("certificate verify failed") == std::string::npos,
        "handshake reports its own protocol failure instead of a stale OpenSSL error");
    plaintext_client.close();
    plaintext_server.close();

    {
        Socket client, server;
        if (!make_test_socket_pair(client, server, err)) {
            check(false, err.c_str());
            return;
        }
        const auto peer_identity = failure_identity();
        if (!peer_identity.complete()) {
            check(false, "create the independent certificate peer identity");
            return;
        }
        client.set_read_timeout(2000, err);
        server.set_read_timeout(2000, err);
        auto peer = std::async(std::launch::async, [&] { return accept_certificate(server, peer_identity); });
        const bool connected = channel.handshake(client, identity, err);
        const bool exchanged = connected && exchange(channel.handle());
        check(exchanged, "certificate handshake and I/O succeed after failures");
        if (!exchanged) {
            client.interrupt();
            server.interrupt();
        }
        check(peer.get(), "existing certificate acceptance policy accepts the independent peer");
        channel = TlsChannel { };
    }

    for (int failure = 0; failure < 7; ++failure) {
        Socket client, server;
        if (!make_test_socket_pair(client, server, err)) {
            check(false, err.c_str());
            return;
        }
        client.set_read_timeout(2000, err);
        server.set_read_timeout(2000, err);
        auto peer = std::async(std::launch::async, [&] { return accept_psk(server); });
        const bool connected = channel.handshake_psk(client, key, err);
        const bool exchanged = connected && exchange(channel.handle());
        check(exchanged, "failed TLS channel can retry a successful PSK handshake and I/O");
        if (!exchanged) {
            client.interrupt();
            server.interrupt();
        }
        check(peer.get(), "retry peer authenticates the PSK and receives bytes");
        if (!exchanged) {
            return;
        }

        if (failure == 6) {
            channel = TlsChannel { };
            check(!channel.valid() && channel.handle() == nullptr, "empty move assignment releases the final successful retry");
            break;
        }

        // 从已建立的连接重新调用握手；即使替换失败，原句柄也不应继续有效。
        auto bad_identity = identity;
        bool accepted = false;
        switch (failure) {
        case 0:
            accepted = channel.handshake_psk(invalid, { }, err);
            break;
        case 1:
            accepted = channel.handshake_psk(invalid, key, err);
            break;
        case 2:
            bad_identity.root_cert = { 'b', 'a', 'd' };
            accepted = channel.handshake(invalid, bad_identity, err);
            break;
        case 3:
            bad_identity.host_cert = { 'b', 'a', 'd' };
            accepted = channel.handshake(invalid, bad_identity, err);
            break;
        case 4:
            bad_identity.host_key = { 'b', 'a', 'd' };
            accepted = channel.handshake(invalid, bad_identity, err);
            break;
        case 5:
            accepted = channel.handshake(invalid, identity, err);
            break;
        }
        check(
            !accepted && !err.empty() && !channel.valid() && channel.handle() == nullptr,
            "failed replacement clears the established and partial TLS handles");
        if (failure >= 2 && failure <= 4) {
            check(
                err.find(failure == 4 ? "private key PEM" : "certificate PEM") != std::string::npos,
                "invalid PEM reports the failed parsing stage");
        }
    }
}

// 销毁移动来源后，先验证现有 TLS 收发，再通过 SSL_clear 开始新的握手。
// 第二次握手强制重新调用 PSK 回调，不会仅因第一次握手成功而漏掉 ex_data 悬空指针。
template <typename Check>
void tls_psk_moves(Check check)
{
    using namespace maa::ios::transport;
    for (bool assignment : { false, true }) {
        Socket client, server;
        std::string err;
        if (!make_test_socket_pair(client, server, err)) {
            check(false, err.c_str());
            return;
        }
        client.set_read_timeout(2000, err);
        server.set_read_timeout(2000, err);
        auto peer = std::async(std::launch::async, [&] { return accept_psk(server); });
        auto original = std::make_unique<TlsChannel>();
        if (!original->handshake_psk(client, { move_psk.begin(), move_psk.end() }, err)) {
            check(false, err.c_str());
            client.interrupt();
            server.interrupt();
            peer.get();
            continue;
        }
        std::unique_ptr<TlsChannel> moved;
        if (assignment) {
            moved = std::make_unique<TlsChannel>();
            *moved = std::move(*original);
        }
        else {
            moved = std::make_unique<TlsChannel>(std::move(*original));
        }
        original.reset();
        const bool first = exchange(moved->handle());
        check(first, assignment ? "move assignment retains established TLS I/O" : "move construction retains established TLS I/O");
        if (!first) {
            client.interrupt();
            server.interrupt();
        }
        check(peer.get(), "PSK peer authenticates and exchanges bytes");
        SSL_shutdown(moved->handle());
        Socket next_client, next_server;
        if (!make_test_socket_pair(next_client, next_server, err)) {
            check(false, err.c_str());
            continue;
        }
        next_client.set_read_timeout(2000, err);
        next_server.set_read_timeout(2000, err);
        auto next_peer = std::async(std::launch::async, [&] { return accept_psk(next_server); });
        const bool connected = SSL_clear(moved->handle()) == 1 && SSL_set_session(moved->handle(), nullptr) == 1
                               && SSL_set_fd(moved->handle(), static_cast<int>(next_client.fd())) == 1 && SSL_connect(moved->handle()) == 1;
        const bool second = connected && exchange(moved->handle());
        check(
            second,
            assignment ? "PSK callback survives move assignment and source destruction"
                       : "PSK callback survives move construction and source destruction");
        if (!second) {
            next_client.interrupt();
            next_server.interrupt();
        }
        check(next_peer.get(), "second handshake authenticates the original PSK");
        // TlsChannel 不拥有 socket，必须在新的 socket 被析构之前释放 TLS。
        moved.reset();
    }
}
} // namespace maa_ios_test
