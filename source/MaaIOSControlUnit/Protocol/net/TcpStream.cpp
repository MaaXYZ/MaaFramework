#include "net/TcpStream.h"
#include "net/LwipRuntime.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
extern "C"
{
#include "lwip/tcp.h"
}

namespace maa::ios::net
{
struct TcpStream::Impl
{
    Stack& stack;
    tcp_pcb* pcb = nullptr; // 只在核心线程访问。
    // mutex 保护应用可见的状态和接收缓存，send_mutex 只串行化应用发送。
    // 持有 mutex 时不能同步调用核心线程；核心回调也需要取得这把锁。
    std::mutex mutex, send_mutex;
    std::condition_variable cv;
    bool started = false, established = false, eof = false, closed = false;
    uint64_t progress = 0;
    std::string failure;
    std::vector<uint8_t> received;

    explicit Impl(Stack& owner_stack)
        : stack(owner_stack)
    {
    }

    void fail(const std::string& reason)
    {
        // 从核心线程撤销 PCB，再发布失败并唤醒等待者；解绑回调后 abort，避免重复访问。
        auto* p = pcb;
        pcb = nullptr;
        {
            std::lock_guard lock(mutex);
            failure = reason;
            established = false;
            ++progress;
        }
        cv.notify_all();
        if (p) {
            tcp_arg(p, nullptr);
            tcp_err(p, nullptr);
            tcp_abort(p);
        }
    }

    static void error(void* arg, err_t code)
    {
        auto& self = *static_cast<Impl*>(arg);
        self.pcb = nullptr; // 错误回调执行前 lwIP 已释放 PCB，不能再次 abort。
        self.fail("TCP connection terminated (lwIP " + std::to_string(code) + ")");
    }

    static err_t connected(void* arg, tcp_pcb*, err_t code)
    {
        auto& self = *static_cast<Impl*>(arg);
        {
            std::lock_guard lock(self.mutex);
            self.established = code == ERR_OK;
            ++self.progress;
        }
        self.cv.notify_all();
        return code;
    }

    static err_t receive(void* arg, tcp_pcb*, pbuf* p, err_t code)
    {
        auto& self = *static_cast<Impl*>(arg);
        if (code != ERR_OK) {
            return code;
        }
        {
            std::lock_guard lock(self.mutex);
            if (!p) {
                self.eof = true;
            }
            else {
                // 接收缓存最多 4 MiB；拒绝本次交付时不释放 pbuf，由 lwIP 保留处理。
                if (self.received.size() + p->tot_len > (4u << 20)) {
                    return ERR_MEM;
                }
                try {
                    const auto offset = self.received.size();
                    self.received.resize(offset + p->tot_len);
                    pbuf_copy_partial(p, self.received.data() + offset, p->tot_len, 0);
                }
                catch (const std::bad_alloc&) {
                    return ERR_MEM;
                }
                pbuf_free(p);
                // 应用取走字节后才调用 tcp_recved；慢读者通过接收窗口形成背压。
            }
            ++self.progress;
        }
        self.cv.notify_all();
        return ERR_OK;
    }

    static err_t sent(void* arg, tcp_pcb*, u16_t)
    {
        auto& self = *static_cast<Impl*>(arg);
        {
            std::lock_guard lock(self.mutex);
            ++self.progress;
        }
        self.cv.notify_all();
        return ERR_OK;
    }
};

TcpStream::TcpStream(Stack& stack)
    : impl_(std::make_shared<Impl>(stack))
{
}

TcpStream::~TcpStream()
{
    close();
}

bool TcpStream::connect(uint16_t port, std::string& err)
{
    auto state = impl_;
    err.clear();
    const auto code = LwipRuntime::instance().call([&]() -> err_t {
        // close 可能先于 connect 的核心任务执行；已关闭端点不能再创建 PCB 或发 SYN。
        {
            std::lock_guard lock(state->mutex);
            if (state->closed) {
                return ERR_CLSD;
            }
        }
        auto* nic = state->stack.network_interface();
        if (!nic || !port || state->started) {
            return ERR_ARG;
        }
        state->started = true;
        state->pcb = tcp_new_ip_type(IPADDR_TYPE_V6);
        if (!state->pcb) {
            return ERR_MEM;
        }
        state->stack.attach_endpoint(state.get(), [state](const std::string& why) { state->fail(why); });
        tcp_arg(state->pcb, state.get());
        tcp_recv(state->pcb, Impl::receive);
        tcp_err(state->pcb, Impl::error);
        tcp_sent(state->pcb, Impl::sent);
        tcp_nagle_disable(state->pcb);
        tcp_bind_netif(state->pcb, nic);
        ip_addr_t local { }, peer { };
        ipaddr_aton(state->stack.local_text().c_str(), &local);
        ipaddr_aton(state->stack.peer_text().c_str(), &peer);
        auto result = tcp_bind(state->pcb, &local, 0);
        if (result == ERR_OK) {
            result = tcp_connect(state->pcb, &peer, port, Impl::connected);
        }
        if (result != ERR_OK) {
            state->fail("TCP connect failed (lwIP " + std::to_string(result) + ")");
        }
        return result;
    });
    if (code != ERR_OK) {
        return err = "Cannot create TCP connection (lwIP " + std::to_string(code) + ")", false;
    }
    std::unique_lock lock(state->mutex);
    const bool ready =
        state->cv.wait_for(lock, std::chrono::seconds(15), [&] { return state->established || state->closed || !state->failure.empty(); });
    if (ready && state->established && !state->closed) {
        return true;
    }
    err = state->failure.empty() ? "TCP connect timed out or connection closed" : state->failure;
    // 失败收尾需要进入核心线程，先释放应用状态锁，避免与 fail() 的锁形成等待环。
    lock.unlock();
    LwipRuntime::instance().call([state, why = err] { state->fail(why); });
    return false;
}

bool TcpStream::send(std::string_view data, std::string& err)
{
    auto state = impl_;
    std::lock_guard serial(state->send_mutex);
    err.clear();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
    size_t offset = 0;
    while (offset < data.size()) {
        uint64_t progress;
        {
            std::lock_guard lock(state->mutex);
            progress = state->progress;
        }
        const auto result = LwipRuntime::instance().call([&] {
            {
                std::lock_guard lock(state->mutex);
                if (state->closed || !state->established || !state->failure.empty() || !state->pcb) {
                    return std::pair<size_t, err_t> { 0, ERR_CONN };
                }
            }
            // 每次最多提交 16 KiB，并复制数据到 lwIP 缓冲；已提交片段不再引用应用缓冲。
            const auto count = static_cast<u16_t>(std::min<size_t>({ data.size() - offset, tcp_sndbuf(state->pcb), 16384 }));
            err_t code = count ? tcp_write(state->pcb, data.data() + offset, count, TCP_WRITE_FLAG_COPY) : ERR_MEM;
            const size_t accepted = code == ERR_OK ? count : 0;
            const auto output = tcp_output(state->pcb);
            if (output != ERR_OK && output != ERR_MEM) {
                code = output;
            }
            return std::pair<size_t, err_t> { accepted, code };
        });
        offset += result.first;
        if (result.second != ERR_OK && result.second != ERR_MEM) {
            std::lock_guard lock(state->mutex);
            return err = state->failure.empty() ? "TCP send failed (connection closed or network error)" : state->failure, false;
        }
        if (offset == data.size()) {
            return true;
        }
        std::unique_lock lock(state->mutex);
        if (std::chrono::steady_clock::now() >= deadline) {
            err = "TCP send timed out";
            lock.unlock();
            LwipRuntime::instance().call([state, why = err] { state->fail(why); });
            return false;
        }
        state->cv.wait_until(lock, std::min(deadline, std::chrono::steady_clock::now() + std::chrono::milliseconds(50)), [&] {
            return state->progress != progress || state->closed || !state->failure.empty();
        });
    }
    return connected() ? true : (err = "TCP connection not established", false);
}

bool TcpStream::recv(std::vector<uint8_t>& out, int timeout_ms, std::string& err, bool* timed_out)
{
    auto state = impl_;
    out.clear();
    err.clear();
    if (timed_out) {
        *timed_out = false;
    }
    std::unique_lock lock(state->mutex);
    if (!state->cv.wait_for(lock, std::chrono::milliseconds(std::max(timeout_ms, 0)), [&] {
            return !state->received.empty() || state->eof || state->closed || !state->failure.empty();
        })) {
        if (timed_out) {
            *timed_out = true;
        }
        else {
            err = "TCP read timed out";
        }
        return false;
    }
    if (state->received.empty()) {
        return err = state->failure.empty() ? "TCP peer or local endpoint closed" : state->failure, false;
    }
    out.swap(state->received);
    lock.unlock();
    // 已交付应用的字节才归还接收窗口；PCB 可能已关闭，因此在核心线程重新检查。
    LwipRuntime::instance().call([state, count = out.size()] {
        if (!state->pcb) {
            return;
        }
        size_t remaining = count;
        while (remaining) {
            const auto n = static_cast<u16_t>(std::min<size_t>(remaining, 65535));
            tcp_recved(state->pcb, n);
            remaining -= n;
        }
    });
    return true;
}

void TcpStream::close()
{
    auto state = impl_;
    LwipRuntime::instance().call([state] {
        {
            std::lock_guard lock(state->mutex);
            state->closed = true;
            state->established = false;
            ++state->progress;
        }
        state->cv.notify_all();
        // 正常关闭交给 Stack 跟踪 FIN/ACK；Impl 注册随后解除，回调不会再引用本端点。
        if (state->pcb) {
            auto* p = state->pcb;
            state->pcb = nullptr;
            state->stack.close_tcp(p);
        }
        state->stack.detach_endpoint(state.get());
    });
}

bool TcpStream::connected() const
{
    std::lock_guard lock(impl_->mutex);
    return impl_->established && !impl_->closed && impl_->failure.empty();
}
} // namespace maa::ios::net
