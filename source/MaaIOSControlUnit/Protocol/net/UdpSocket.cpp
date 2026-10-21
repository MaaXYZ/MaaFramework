#include "net/UdpSocket.h"
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

    Impl(Stack& owner_stack, uint16_t local_port)
        : stack(owner_stack)
        , port(local_port)
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

UdpSocket::UdpSocket(Stack& stack, uint16_t port)
    : impl_(std::make_shared<Impl>(stack, port))
{
}

UdpSocket::~UdpSocket()
{
    auto state = impl_;
    LwipRuntime::instance().call([state] {
        // 移除 PCB 并解除注册，在同一个核心操作中完成；之后不再有引用 Impl 的协议回调。
        state->fail("UDP endpoint closed");
        state->stack.detach_endpoint(state.get());
    });
}

bool UdpSocket::bind(std::string& err)
{
    auto state = impl_;
    err.clear();
    const auto status = LwipRuntime::instance().call([state]() -> err_t {
        auto* nic = state->stack.network_interface();
        if (!nic || !state->port || state->pcb) {
            return ERR_ARG;
        }
        state->pcb = udp_new_ip_type(IPADDR_TYPE_V6);
        if (!state->pcb) {
            return ERR_MEM;
        }
        ip_addr_t address { };
        ipaddr_aton(state->stack.local_text().c_str(), &address);
        udp_bind_netif(state->pcb, nic);
        const auto bind_result = udp_bind(state->pcb, &address, state->port);
        if (bind_result != ERR_OK) {
            udp_remove(state->pcb);
            state->pcb = nullptr;
            return bind_result;
        }
        udp_recv(state->pcb, Impl::received, state.get());
        state->stack.attach_endpoint(state.get(), [state](const std::string& why) { state->fail(why); });
        return ERR_OK;
    });
    return status == ERR_OK ? true : (err = "UDP bind failed (lwIP " + std::to_string(status) + ")", false);
}

bool UdpSocket::send(const std::vector<uint8_t>& payload, uint16_t port, std::string& err)
{
    auto state = impl_;
    err.clear();
    const auto status = LwipRuntime::instance().call([&]() -> err_t {
        auto* nic = state->stack.network_interface();
        if (!nic || !state->pcb || !port) {
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
        ipaddr_aton(state->stack.peer_text().c_str(), &peer);
        const auto send_result = udp_sendto(state->pcb, p, &peer, port);
        pbuf_free(p);
        return send_result;
    });
    return status == ERR_OK ? true : (err = "UDP send failed (lwIP " + std::to_string(status) + ")", false);
}

bool UdpSocket::recv(std::vector<uint8_t>& payload, uint16_t& port, int timeout_ms, std::string& err)
{
    auto state = impl_;
    payload.clear();
    err.clear();
    std::unique_lock lock(state->mutex);
    if (!state->cv.wait_for(lock, std::chrono::milliseconds(std::max(timeout_ms, 0)), [&] {
            return !state->queue.empty() || !state->failure.empty();
        })) {
        return err = "UDP receive timed out", false;
    }
    if (state->queue.empty()) {
        return err = state->failure, false;
    }
    auto packet = std::move(state->queue.front());
    state->queue.pop_front();
    state->bytes -= packet.bytes.size();
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
