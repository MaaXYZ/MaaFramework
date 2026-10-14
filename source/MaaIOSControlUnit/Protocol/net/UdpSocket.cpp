#include "net/UdpSocket.h"
#include "i18n/Translation.h"
#include "net/LwipRuntime.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
extern "C"
{
#include "lwip/udp.h"
}

namespace maa::ios::net
{
struct UdpSocket::Impl
{
    struct Packet
    {
        uint16_t port;
        std::vector<uint8_t> bytes;
    };

    Stack& stack;
    uint16_t port;
    udp_pcb* pcb = nullptr; // 仅由 lwIP 核心线程及其回调访问。
    // mutex 保护应用可见的队列和错误；持锁期间不等待核心线程。
    mutable std::mutex mutex;
    std::condition_variable cv;
    std::deque<Packet> queue;
    size_t bytes = 0, dropped = 0;
    std::string failure;

    Impl(Stack& s, uint16_t p)
        : stack(s)
        , port(p)
    {
    }

    void fail(const std::string& why)
    {
        // 核心线程先移除 PCB，再发布失败；已缓存的数据报仍可由 recv 取走。
        if (pcb) {
            udp_remove(pcb);
            pcb = nullptr;
        }
        {
            std::lock_guard lock(mutex);
            failure = why;
        }
        cv.notify_all();
    }

    static void received(void* arg, udp_pcb*, pbuf* p, const ip_addr_t* address, u16_t port)
    {
        auto& self = *static_cast<Impl*>(arg);
        ip_addr_t peer { };
        ipaddr_aton(self.stack.peer_text().c_str(), &peer);
        // 隧道可能传入其它地址的数据报，应用端点只接收协商的对端地址。
        if (!ip_addr_cmp(address, &peer)) {
            pbuf_free(p);
            return;
        }
        {
            std::lock_guard lock(self.mutex);
            try {
                std::vector<uint8_t> data(p->tot_len);
                pbuf_copy_partial(p, data.data(), p->tot_len, 0);
                // 保留较新的媒体数据，丢弃队头；UDP 无接收窗口，不在回调里等待消费者。
                while (!self.queue.empty() && (self.queue.size() >= 4096 || self.bytes + data.size() > (16u << 20))) {
                    self.bytes -= self.queue.front().bytes.size();
                    self.queue.pop_front();
                    ++self.dropped;
                }
                self.queue.push_back({ port, std::move(data) });
                self.bytes += p->tot_len;
            }
            catch (const std::bad_alloc&) {
                ++self.dropped;
            }
        }
        pbuf_free(p);
        self.cv.notify_all();
    }
};

UdpSocket::UdpSocket(Stack& s, uint16_t port)
    : impl_(std::make_shared<Impl>(s, port))
{
}

UdpSocket::~UdpSocket()
{
    auto s = impl_;
    LwipRuntime::instance().call([s] {
        // 移除 PCB 并解除注册，在同一个核心操作中完成；之后不再有引用 Impl 的协议回调。
        s->fail(MAA_IOS_TR("UDP endpoint closed"));
        s->stack.detach_endpoint(s.get());
    });
}

bool UdpSocket::bind(std::string& err)
{
    auto s = impl_;
    err.clear();
    const auto result = LwipRuntime::instance().call([s]() -> err_t {
        auto* nic = s->stack.network_interface();
        if (!nic || !s->port || s->pcb) {
            return ERR_ARG;
        }
        s->pcb = udp_new_ip_type(IPADDR_TYPE_V6);
        if (!s->pcb) {
            return ERR_MEM;
        }
        ip_addr_t address { };
        ipaddr_aton(s->stack.local_text().c_str(), &address);
        udp_bind_netif(s->pcb, nic);
        const auto result = udp_bind(s->pcb, &address, s->port);
        if (result != ERR_OK) {
            udp_remove(s->pcb);
            s->pcb = nullptr;
            return result;
        }
        udp_recv(s->pcb, Impl::received, s.get());
        s->stack.attach_endpoint(s.get(), [s](const std::string& why) { s->fail(why); });
        return ERR_OK;
    });
    return result == ERR_OK ? true : (err = MAA_IOS_TR("UDP bind failed (lwIP ") + std::to_string(result) + MAA_IOS_TR(")"), false);
}

bool UdpSocket::send(const std::vector<uint8_t>& payload, uint16_t port, std::string& err)
{
    auto s = impl_;
    err.clear();
    const auto result = LwipRuntime::instance().call([&]() -> err_t {
        auto* nic = s->stack.network_interface();
        if (!nic || !s->pcb || !port) {
            return ERR_CONN;
        }
        // 为 IPv6 头和 UDP 头预留 40+8 字节，拒绝超过隧道 MTU 的数据报。
        if (payload.size() > nic->mtu - 48u) {
            return ERR_VAL;
        }
        pbuf* p = pbuf_alloc(PBUF_TRANSPORT, static_cast<u16_t>(payload.size()), PBUF_RAM);
        if (!p) {
            return ERR_MEM;
        }
        pbuf_take(p, payload.data(), static_cast<u16_t>(payload.size()));
        ip_addr_t peer { };
        ipaddr_aton(s->stack.peer_text().c_str(), &peer);
        const auto result = udp_sendto(s->pcb, p, &peer, port);
        pbuf_free(p);
        return result;
    });
    return result == ERR_OK ? true : (err = MAA_IOS_TR("UDP send failed (lwIP ") + std::to_string(result) + MAA_IOS_TR(")"), false);
}

bool UdpSocket::recv(std::vector<uint8_t>& payload, uint16_t& port, int timeout_ms, std::string& err)
{
    auto s = impl_;
    payload.clear();
    err.clear();
    std::unique_lock lock(s->mutex);
    if (!s->cv.wait_for(lock, std::chrono::milliseconds(std::max(timeout_ms, 0)), [&] {
            return !s->queue.empty() || !s->failure.empty();
        })) {
        return err = MAA_IOS_TR("UDP receive timed out"), false;
    }
    if (s->queue.empty()) {
        return err = s->failure, false;
    }
    auto packet = std::move(s->queue.front());
    s->queue.pop_front();
    s->bytes -= packet.bytes.size();
    payload = std::move(packet.bytes);
    port = packet.port;
    return true;
}

uint16_t UdpSocket::local_port() const
{
    return impl_->port;
}

size_t UdpSocket::buffered() const
{
    std::lock_guard lock(impl_->mutex);
    return impl_->queue.size();
}

size_t UdpSocket::dropped() const
{
    std::lock_guard lock(impl_->mutex);
    return impl_->dropped;
}
} // namespace maa::ios::net
