#include "ddi/ServiceTransport.h"
#include "remote/Device.h"
#include "transport/Lockdown.h"
#include "transport/Usbmux.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <limits>
#include <mutex>
#include <thread>

namespace maa::ios::ddi
{
namespace
{
using Clock = std::chrono::steady_clock;
using Ms = std::chrono::milliseconds;
using V = plist::Value;
constexpr uint32_t kMaxFrame = 8u << 20;
constexpr char kUsbService[] = "com.apple.mobile.mobile_image_mounter";
constexpr char kRemoteService[] = "com.apple.mobile.mobile_image_mounter.shim.remote";

Clock::time_point deadline(Ms timeout)
{
    return Clock::now() + std::clamp(timeout, Ms(0), Ms(std::chrono::hours(1)));
}

Ms remaining(Clock::time_point until)
{
    return std::max(Ms(0), std::chrono::duration_cast<Ms>(until - Clock::now()));
}

// The watched resource remains at a fixed address. Stop/join occurs before
// ownership transfer, reconnect, TLS release or destruction; interrupt never frees it.
class Watchdog
{
public:
    Watchdog(Ms timeout, const std::function<bool()>& cancelled, std::function<void()> interrupt)
        : until_(deadline(timeout))
        , cancelled_(cancelled)
        , interrupt_(std::move(interrupt))
    {
        check();
        if (reason_.load() == None) {
            thread_ = std::thread([this] {
                std::unique_lock lock(mutex_);
                while (!done_) {
                    cv_.wait_until(lock, std::min(until_, Clock::now() + Ms(25)), [this] { return done_; });
                    if (done_) {
                        break;
                    }
                    lock.unlock();
                    check();
                    lock.lock();
                    if (reason_.load() != None) {
                        break;
                    }
                }
            });
        }
    }

    ~Watchdog() { stop(); }

    bool active() const { return reason_.load() == None; }

    bool finish(std::string& error)
    {
        stop();
        check();
        switch (reason_.load()) {
        case Timeout:
            error = "DDI service operation timed out";
            return false;
        case Cancelled:
            error = "DDI service operation was cancelled";
            return false;
        case Callback:
            error = "DDI service cancellation callback failed";
            return false;
        default:
            return true;
        }
    }

private:
    enum Reason
    {
        None,
        Timeout,
        Cancelled,
        Callback
    };

    void check() noexcept
    {
        if (reason_.load() != None) {
            return;
        }
        Reason reason = None;
        try {
            if (cancelled_ && cancelled_()) {
                reason = Cancelled;
            }
            else if (Clock::now() >= until_) {
                reason = Timeout;
            }
        }
        catch (...) {
            reason = Callback;
        }
        if (reason != None) {
            reason_.store(reason);
            try {
                interrupt_();
            }
            catch (...) {
                reason_.store(Callback);
            }
        }
    }

    void stop()
    {
        {
            std::lock_guard lock(mutex_);
            done_ = true;
        }
        cv_.notify_all();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    Clock::time_point until_;
    const std::function<bool()>& cancelled_;
    std::function<void()> interrupt_;
    std::atomic<Reason> reason_ { None };
    std::mutex mutex_;
    std::condition_variable cv_;
    bool done_ = false;
    std::thread thread_;
};
} // namespace

struct ServiceTransport::Impl
{
    transport::Lockdown* lockdown = nullptr;
    remote::Device* device = nullptr;
    uint32_t device_id = 0;
    std::function<bool()> cancelled;
    transport::Socket socket;
    transport::TlsChannel tls;
    std::unique_ptr<net::TcpStream> tcp;
    net::ByteStream* test_stream = nullptr;
    std::function<void()> test_interrupt;
    std::vector<uint8_t> pending;
    size_t offset = 0;

    ~Impl() { socket.interrupt(); }

    std::function<void()> interrupter()
    {
        if (tcp) {
            auto* stream = tcp.get();
            return [stream] {
                stream->close();
            };
        }
        if (test_stream) {
            return test_interrupt;
        }
        return [this] {
            socket.interrupt();
        };
    }

    bool write(const uint8_t* bytes, size_t count, std::string& error)
    {
        auto* stream = tcp ? static_cast<net::ByteStream*>(tcp.get()) : test_stream;
        if (stream) {
            return stream->send(std::string_view(reinterpret_cast<const char*>(bytes), count), error);
        }
        auto* ssl = tls.handle();
        if (!ssl) {
            return socket.write_all(bytes, count, error);
        }
        size_t write_offset = 0;
        while (write_offset < count) {
            const auto length = static_cast<int>(std::min(count - write_offset, size_t(INT_MAX)));
            const auto written = SSL_write(ssl, bytes + write_offset, length);
            if (written > 0) {
                write_offset += static_cast<size_t>(written);
                continue;
            }
            const auto code = SSL_get_error(ssl, written);
            if (code == SSL_ERROR_WANT_READ || code == SSL_ERROR_WANT_WRITE) {
                if (transport::wait_socket(socket.fd(), code == SSL_ERROR_WANT_WRITE, 25) >= 0) {
                    continue;
                }
            }
            error = "Cannot write DDI service TLS data";
            return false;
        }
        return true;
    }

    bool read(uint8_t* bytes, size_t count, std::string& error)
    {
        auto* stream = tcp ? static_cast<net::ByteStream*>(tcp.get()) : test_stream;
        if (stream) {
            size_t copied = 0;
            while (copied < count) {
                const auto available = pending.size() - offset;
                if (available) {
                    const auto n = std::min(available, count - copied);
                    std::memcpy(bytes + copied, pending.data() + offset, n);
                    copied += n;
                    offset += n;
                    if (offset == pending.size()) {
                        pending.clear();
                        offset = 0;
                    }
                    continue;
                }
                bool timed_out = false;
                if (!stream->recv(pending, 25, error, &timed_out)) {
                    if (timed_out) {
                        continue;
                    }
                    return false;
                }
                if (pending.empty()) {
                    error = "DDI service closed the connection";
                    return false;
                }
            }
            return true;
        }
        auto* ssl = tls.handle();
        if (!ssl) {
            return socket.read_exact(bytes, count, error);
        }
        size_t copied = 0;
        while (copied < count) {
            const auto length = static_cast<int>(std::min(count - copied, size_t(INT_MAX)));
            const auto n = SSL_read(ssl, bytes + copied, length);
            if (n > 0) {
                copied += static_cast<size_t>(n);
                continue;
            }
            const auto code = SSL_get_error(ssl, n);
            if (code == SSL_ERROR_WANT_READ || code == SSL_ERROR_WANT_WRITE) {
                if (transport::wait_socket(socket.fd(), code == SSL_ERROR_WANT_WRITE, 25) >= 0) {
                    continue;
                }
            }
            error = "Cannot read DDI service TLS data";
            return false;
        }
        return true;
    }

    bool write_frame(const V& request, std::string& error)
    {
        const auto body = plist::write(request);
        if (body.empty() || body.size() > kMaxFrame) {
            error = "DDI service request exceeds its frame limit";
            return false;
        }
        const auto length = static_cast<uint32_t>(body.size());
        const uint8_t header[] { static_cast<uint8_t>(length >> 24),
                                 static_cast<uint8_t>(length >> 16),
                                 static_cast<uint8_t>(length >> 8),
                                 static_cast<uint8_t>(length) };
        return write(header, sizeof(header), error) && write(reinterpret_cast<const uint8_t*>(body.data()), body.size(), error);
    }

    std::optional<V> read_frame(std::string& error)
    {
        uint8_t header[4];
        if (!read(header, sizeof(header), error)) {
            return { };
        }
        const auto length = uint32_t(header[0]) << 24 | uint32_t(header[1]) << 16 | uint32_t(header[2]) << 8 | header[3];
        if (!length || length > kMaxFrame) {
            error = "DDI service response has an invalid frame length";
            return { };
        }
        std::string body(length, '\0');
        if (!read(reinterpret_cast<uint8_t*>(body.data()), body.size(), error)) {
            return { };
        }
        auto result = plist::parse(body);
        if (!result || !result->is_dict()) {
            error = "DDI service response is not a plist dictionary";
            return { };
        }
        return result;
    }

    bool connect(Ms timeout, std::string& error)
    {
        error.clear();
        const auto until = deadline(timeout);
        // No watchdog is running while replacing ownership.
        tcp.reset();
        socket.interrupt();
        tls = transport::TlsChannel { };
        socket.close();
        pending.clear();
        offset = 0;
        if (lockdown) {
            std::optional<transport::Lockdown::ServiceEndpoint> endpoint;
            {
                Watchdog watch(remaining(until), cancelled, [this] { lockdown->interrupt(); });
                if (watch.active()) {
                    endpoint = lockdown->start_service(kUsbService, error);
                }
                if (!watch.finish(error) || !endpoint || !endpoint->port) {
                    return false;
                }
            }
            auto mux = transport::Usbmux::open_for_service(remaining(until), cancelled, error);
            if (!mux) {
                return false;
            }
            auto opened = mux->connect_for_service(device_id, endpoint->port, remaining(until), cancelled, error);
            if (!opened) {
                return false;
            }
            socket = std::move(*opened);
            if (endpoint->requires_tls) {
                Watchdog watch(remaining(until), cancelled, [this] { socket.interrupt(); });
                const auto ok = watch.active() && tls.handshake(socket, lockdown->identity(), error);
                if (!watch.finish(error) || !ok) {
                    return false;
                }
            }
            return true;
        }
        if (!device || !device->stack()) {
            error = "DDI remote session is unavailable";
            return false;
        }
        const auto endpoint = device->rsd().service(kRemoteService);
        if (!endpoint || !endpoint->port) {
            error = "DDI remote image-mounter service is unavailable";
            return false;
        }
        if (endpoint->uses_remote_xpc || endpoint->encrypt_socket_data) {
            error = "DDI remote image-mounter service uses an unsupported transport";
            return false;
        }
        tcp = std::make_unique<net::TcpStream>(*device->stack());
        bool ok = false;
        {
            Watchdog watch(remaining(until), cancelled, interrupter());
            ok = watch.active() && tcp->connect(endpoint->port, error);
            if (!watch.finish(error) || !ok) {
                return false;
            }
        }
        Watchdog watch(remaining(until), cancelled, interrupter());
        if (!watch.active()) {
            return watch.finish(error);
        }
        auto checkin = V::Dict();
        checkin.set("Label", V::Str("MaaFramework/iOS"));
        checkin.set("ProtocolVersion", V::Str("2"));
        checkin.set("Request", V::Str("RSDCheckin"));
        ok = write_frame(checkin, error);
        for (const auto* expected : { "RSDCheckin", "StartService" }) {
            auto response = ok ? read_frame(error) : std::optional<V> { };
            const auto* request = response ? response->find("Request") : nullptr;
            if (!response || response->find("Error") || !request || !request->is_string() || request->string != expected) {
                if (response) {
                    error = "DDI remote image-mounter check-in was rejected";
                }
                ok = false;
                break;
            }
        }
        return watch.finish(error) && ok;
    }
};

ServiceTransport::ServiceTransport()
    : impl_(std::make_unique<Impl>())
{
}

ServiceTransport::~ServiceTransport() = default;

std::unique_ptr<ServiceTransport> ServiceTransport::open_usb(
    transport::Lockdown& lockdown,
    uint32_t id,
    Ms timeout,
    const std::function<bool()>& cancelled,
    std::string& error)
{
    try {
        auto adapter = std::unique_ptr<ServiceTransport>(new ServiceTransport);
        adapter->impl_->lockdown = &lockdown;
        adapter->impl_->device_id = id;
        adapter->impl_->cancelled = cancelled;
        if (!adapter->impl_->connect(timeout, error)) {
            return { };
        }
        return adapter;
    }
    catch (const std::exception& e) {
        error = e.what();
        return { };
    }
}

std::unique_ptr<ServiceTransport>
    ServiceTransport::open_remote(remote::Device& device, Ms timeout, const std::function<bool()>& cancelled, std::string& error)
{
    try {
        auto adapter = std::unique_ptr<ServiceTransport>(new ServiceTransport);
        adapter->impl_->device = &device;
        adapter->impl_->cancelled = cancelled;
        if (!adapter->impl_->connect(timeout, error)) {
            return { };
        }
        return adapter;
    }
    catch (const std::exception& e) {
        error = e.what();
        return { };
    }
}

PlistService ServiceTransport::service()
{
    PlistService service;
    service.request = [this](const V& request, Ms timeout, std::string& error) -> std::optional<V> {
        error.clear();
        try {
            Watchdog watch(timeout, impl_->cancelled, impl_->interrupter());
            auto result = watch.active() && impl_->write_frame(request, error) ? impl_->read_frame(error) : std::optional<V> { };
            if (!watch.finish(error)) {
                return { };
            }
            return result;
        }
        catch (const std::exception& e) {
            error = e.what();
            return { };
        }
    };
    service.receive = [this](Ms timeout, std::string& error) -> std::optional<V> {
        error.clear();
        try {
            Watchdog watch(timeout, impl_->cancelled, impl_->interrupter());
            auto result = watch.active() ? impl_->read_frame(error) : std::optional<V> { };
            if (!watch.finish(error)) {
                return { };
            }
            return result;
        }
        catch (const std::exception& e) {
            error = e.what();
            return { };
        }
    };
    service.send_bytes = [this](const uint8_t* data, size_t size, Ms timeout, std::string& error) {
        error.clear();
        try {
            if (!data && size) {
                error = "DDI service raw upload data is missing";
                return false;
            }
            Watchdog watch(timeout, impl_->cancelled, impl_->interrupter());
            const auto ok = watch.active() && impl_->write(data, size, error);
            return watch.finish(error) && ok;
        }
        catch (const std::exception& e) {
            error = e.what();
            return false;
        }
    };
    service.reconnect = [this](Ms timeout, std::string& error) {
        try {
            return impl_->connect(timeout, error);
        }
        catch (const std::exception& e) {
            error = e.what();
            return false;
        }
    };
    return service;
}

std::unique_ptr<ServiceTransport> detail::service_transport_with_socket(transport::Socket socket, const std::function<bool()>& cancelled)
{
    auto adapter = std::unique_ptr<ServiceTransport>(new ServiceTransport);
    adapter->impl_->socket = std::move(socket);
    adapter->impl_->cancelled = cancelled;
    return adapter;
}

std::unique_ptr<ServiceTransport> detail::service_transport_with_byte_stream(
    net::ByteStream& stream,
    const std::function<void()>& interrupt,
    const std::function<bool()>& cancelled)
{
    auto adapter = std::unique_ptr<ServiceTransport>(new ServiceTransport);
    adapter->impl_->test_stream = &stream;
    adapter->impl_->test_interrupt = interrupt;
    adapter->impl_->cancelled = cancelled;
    return adapter;
}
} // namespace maa::ios::ddi
