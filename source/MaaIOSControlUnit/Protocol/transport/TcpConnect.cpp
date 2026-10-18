#include "transport/TcpConnect.h"

#ifndef _WIN32
#include <fcntl.h>
#endif

#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>

namespace maa::ios::transport
{

std::optional<Socket> connect_tcp(const std::string& host, uint16_t port, int timeout_ms, std::string& err)
{
    err.clear();
    if (!initialize_sockets(err)) {
        return std::nullopt;
    }
    addrinfo hints { };
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    const std::string service = std::to_string(port);
    addrinfo* first = nullptr;
    const int rc = ::getaddrinfo(host.c_str(), service.c_str(), &hints, &first);
    if (rc != 0 || first == nullptr) {
        err = "Failed to resolve address " + host + ": " + (rc == 0 ? "no results" : gai_strerror(rc));
        return std::nullopt;
    }
    std::unique_ptr<addrinfo, void (*)(addrinfo*)> addresses(first, ::freeaddrinfo);

    // 逐个地址试：一个主机名同时有 A 和 AAAA、或者 v6 先回来但这台机器没 v6 路由，
    // 都是局域网里会遇到的事，只试第一个会变成"偶尔连不上"。
    std::string last_error;
    for (const addrinfo* ai = first; ai != nullptr; ai = ai->ai_next) {
        Socket socket(::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol));
        if (!socket.valid()) {
            last_error = socket_error_message();
            continue;
        }
        // Socket 从创建起拥有 fd，所有失败分支自动关闭，并统一处理 SIGPIPE。
        const NativeSocket fd = socket.fd();
#ifndef _WIN32
        int flags = 0;
#endif
        const bool nonblocking = timeout_ms > 0;
        if (nonblocking) {
#ifdef _WIN32
            u_long mode = 1;
            if (::ioctlsocket(fd, FIONBIO, &mode) != 0) {
#else
            flags = ::fcntl(fd, F_GETFL, 0);
            if (flags < 0 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
#endif
                last_error = "Failed to enable nonblocking mode: " + socket_error_message();
                continue;
            }
        }
        // Native socket lengths differ on Windows. A resolved socket address
        // must fit sockaddr_storage before narrowing to the platform API type.
        if (!ai->ai_addr || ai->ai_addrlen > sizeof(sockaddr_storage)) {
            last_error = "Resolved address exceeds socket storage";
            continue;
        }
        const auto address_length = static_cast<socklen_t>(ai->ai_addrlen);
        if (::connect(fd, ai->ai_addr, address_length) != 0) {
            if (!(socket_connect_pending(socket_error()) && nonblocking)) {
                last_error = socket_error_message();
                continue;
            }
            // EINTR 后只等待剩余时间，避免重试一次就重置整个连接超时。
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
            int ready = 0;
            do {
                const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
                if (remaining <= 0) {
                    ready = 0;
                    break;
                }
                ready = wait_socket(fd, true, static_cast<int>(remaining));
            } while (ready < 0 && socket_error() == kSocketInterrupted);
            if (ready <= 0) {
                last_error = ready == 0 ? "Connection timed out" + std::to_string(timeout_ms) + "ms"
                                        : "Failed to wait for connection: " + std::string(socket_error_message());
                continue;
            }
            int so_error = 0;
            socklen_t len = sizeof(so_error);
            if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&so_error), &len) != 0 || so_error != 0) {
                last_error = socket_error_message(so_error == 0 ? socket_error() : so_error);
                continue;
            }
        }
        // 交出去的 Socket read_exact/write_all 按阻塞 fd 工作，恢复失败就不交付它。
#ifdef _WIN32
        u_long mode = 0;
        if (nonblocking && ::ioctlsocket(fd, FIONBIO, &mode) != 0) {
#else
        if (nonblocking && ::fcntl(fd, F_SETFL, flags) != 0) {
#endif
            last_error = "Failed to restore blocking mode: " + std::string(socket_error_message());
            continue;
        }
        return socket;
    }
    err = "Cannot connect to " + host + ":" + service + (last_error.empty() ? std::string() : "：" + last_error);
    return std::nullopt;
}

namespace
{
std::vector<detail::TcpAddress> resolve_addresses(const std::string& host, uint16_t port, std::string& err, bool numeric = false)
{
    addrinfo hints { };
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (numeric) {
        hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;
    }
    addrinfo* first = nullptr;
    const auto service = std::to_string(port);
    const int rc = ::getaddrinfo(host.c_str(), service.c_str(), &hints, &first);
    std::unique_ptr<addrinfo, void (*)(addrinfo*)> owner(first, ::freeaddrinfo);
    std::vector<detail::TcpAddress> result;
    if (rc != 0 || !first) {
        if (!numeric) {
            err = "Failed to resolve address " + host + ": " + (rc ? gai_strerror(rc) : "no results");
        }
        return result;
    }
    for (auto* ai = first; ai; ai = ai->ai_next) {
        if (ai->ai_addrlen > sizeof(sockaddr_storage)) {
            continue;
        }
        detail::TcpAddress item;
        std::memcpy(&item.address, ai->ai_addr, ai->ai_addrlen);
        item.length = static_cast<socklen_t>(ai->ai_addrlen);
        item.family = ai->ai_family;
        item.type = ai->ai_socktype;
        item.protocol = ai->ai_protocol;
        result.push_back(item);
    }
    return result;
}

struct ResolverSlots
{
    std::atomic<unsigned> active { 0 };
};

// Owned by both callers and workers, including during process/static teardown.
std::shared_ptr<ResolverSlots> resolver_slots()
{
    static auto slots = std::make_shared<ResolverSlots>();
    return slots;
}

struct Resolution
{
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    std::vector<detail::TcpAddress> addresses;
    std::string error;
};
}

std::optional<Socket> detail::connect_tcp_bounded_with_resolver(
    const std::string& host,
    uint16_t port,
    const IoBudget& budget,
    std::string& err,
    TcpResolver resolver)
{
    err.clear();
    if (!budget.check(err) || !initialize_sockets(err)) {
        return { };
    }
    auto addresses = resolve_addresses(host, port, err, true);
    if (!budget.check(err)) {
        return { };
    }
    if (addresses.empty()) {
        auto slots = resolver_slots();
        unsigned count = slots->active.load();
        do {
            if (count >= 2) {
                err = "Too many outstanding hostname resolutions; retry later";
                return { };
            }
        } while (!slots->active.compare_exchange_weak(count, count + 1));
        auto job = std::make_shared<Resolution>();
        try {
            std::thread([job, slots, resolver = std::move(resolver), host, port] {
                std::vector<TcpAddress> found;
                std::string error;
                try {
                    found = resolver(host, port, error);
                }
                catch (...) {
                    error = "Hostname resolution failed";
                }
                {
                    std::lock_guard lock(job->mutex);
                    job->addresses = std::move(found);
                    job->error = std::move(error);
                    job->done = true;
                }
                slots->active.fetch_sub(1);
                job->cv.notify_all();
            }).detach();
        }
        catch (...) {
            slots->active.fetch_sub(1);
            err = "Cannot start hostname resolution";
            return { };
        }
        std::unique_lock lock(job->mutex);
        while (!job->done) {
            if (!budget.check(err)) {
                return { };
            }
            job->cv.wait_for(lock, std::min(budget.remaining(), std::chrono::milliseconds(25)));
        }
        if (!budget.check(err)) {
            return { };
        }
        addresses = std::move(job->addresses);
        if (addresses.empty()) {
            err = job->error.empty() ? "Hostname resolution returned no addresses" : job->error;
            return { };
        }
    }
    std::string last_error;
    for (const auto& address : addresses) {
        if (!budget.check(err)) {
            return { };
        }
        Socket socket(::socket(address.family, address.type, address.protocol));
        if (!socket.valid()) {
            last_error = socket_error_message();
            continue;
        }
        const auto fd = socket.fd();
#ifdef _WIN32
        u_long mode = 1;
        if (::ioctlsocket(fd, FIONBIO, &mode) != 0) {
#else
        const int flags = ::fcntl(fd, F_GETFL, 0);
        if (flags < 0 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
#endif
            last_error = socket_error_message();
            continue;
        }
        if (::connect(fd, reinterpret_cast<const sockaddr*>(&address.address), address.length) != 0) {
            const int code = socket_error();
            if (!socket_connect_pending(code) && code != kSocketInterrupted) {
                last_error = socket_error_message(code);
                continue;
            }
            int ready = 0;
            do {
                if (!budget.check(err)) {
                    return { };
                }
                ready = wait_socket(fd, true, static_cast<int>(std::min(budget.remaining(), std::chrono::milliseconds(25)).count()));
            } while (ready == 0 || (ready < 0 && socket_error() == kSocketInterrupted));
            if (ready < 0) {
                last_error = socket_error_message();
                continue;
            }
            int error = 0;
            socklen_t len = sizeof(error);
            if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &len) != 0 || error) {
                last_error = socket_error_message(error ? error : socket_error());
                continue;
            }
        }
        if (!budget.check(err)) {
            return { };
        }
#ifdef _WIN32
        mode = 0;
        if (::ioctlsocket(fd, FIONBIO, &mode) != 0) {
#else
        if (::fcntl(fd, F_SETFL, flags) != 0) {
#endif
            last_error = socket_error_message();
            continue;
        }
        if (!budget.check(err)) {
            return { };
        }
        return socket;
    }
    if (!budget.check(err)) {
        return { };
    }
    err = "Cannot connect to " + host + ":" + std::to_string(port) + (last_error.empty() ? std::string { } : ": " + last_error);
    return { };
}

std::optional<Socket> connect_tcp_bounded(const std::string& host, uint16_t port, const IoBudget& budget, std::string& err)
{
    return detail::connect_tcp_bounded_with_resolver(
        host,
        port,
        budget,
        err,
        [](const std::string& name, uint16_t service, std::string& error) { return resolve_addresses(name, service, error); });
}

} // namespace maa::ios::transport
