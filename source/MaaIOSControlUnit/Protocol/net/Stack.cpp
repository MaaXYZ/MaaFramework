#include "net/Stack.h"
#include "i18n/Translation.h"

#include "net/LwipRuntime.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <deque>
#include <map>
#include <set>
#include <utility>
extern "C"
{
#include "lwip/ip6.h"
#include "lwip/netif.h"
#include "lwip/tcp.h"
}

namespace maa::ios::net
{
namespace
{

uint16_t get16(const uint8_t* p)
{
    return static_cast<uint16_t>(p[0] << 8 | p[1]);
}

uint32_t fold_sum(uint32_t sum, const uint8_t* p, std::size_t n)
{
    for (std::size_t i = 0; i + 1 < n; i += 2) {
        sum += get16(p + i);
    }
    if (n & 1) {
        sum += static_cast<uint32_t>(p[n - 1]) << 8;
    }
    return sum;
}

uint16_t finish_sum(uint32_t sum)
{
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return static_cast<uint16_t>(~sum & 0xFFFF);
}

} // namespace

uint16_t l4_checksum(const uint8_t src[16], const uint8_t dst[16], const uint8_t* l4, std::size_t len, uint8_t next_header)
{
    uint32_t sum = 0;
    sum = fold_sum(sum, src, 16);
    sum = fold_sum(sum, dst, 16);
    // IPv6 伪头包含 4 字节上层长度、3 字节零和 next header；与 L4 数据
    // 连续累加，最后统一折叠进位并取反。
    const uint8_t pseudo[8] = {
        static_cast<uint8_t>(len >> 24),
        static_cast<uint8_t>(len >> 16),
        static_cast<uint8_t>(len >> 8),
        static_cast<uint8_t>(len),
        0,
        0,
        0,
        next_header,
    };
    sum = fold_sum(sum, pseudo, sizeof(pseudo));
    sum = fold_sum(sum, l4, len);
    return finish_sum(sum);
}

struct Stack::Impl
{
    netif nic { };
    bool registered = false;
    Stack* owner;
    // 只有出站队列跨核心线程与隧道线程；netif、端点表和关闭记录由核心线程独占。
    std::mutex queue_mutex;
    std::deque<std::vector<uint8_t>> outgoing;
    size_t queued_bytes = 0;
    std::map<const void*, std::function<void(const std::string&)>> endpoints;
    std::set<tcp_pcb*> closing;

    static err_t init(netif* nic)
    {
        nic->name[0] = 's';
        nic->name[1] = 'c';
        nic->output_ip6 = output;
        return ERR_OK;
    }

    static err_t output(netif* nic, pbuf* p, const ip6_addr_t*)
    {
        auto& self = *static_cast<Impl*>(nic->state);
        try {
            std::vector<uint8_t> packet(p->tot_len);
            if (pbuf_copy_partial(p, packet.data(), p->tot_len, 0) != p->tot_len) {
                return ERR_BUF;
            }
            // output 回调不能阻塞在 TLS 写入，只复制并提交包；队列满时反馈 ERR_MEM。
            return static_cast<err_t>(self.owner->enqueue(std::move(packet)) ? ERR_OK : ERR_MEM);
        }
        catch (const std::bad_alloc&) {
            return ERR_MEM;
        }
    }

    struct Closing
    {
        Impl* owner;
        tcp_pcb* pcb;

        static void destroyed(u8_t, void* arg)
        {
            auto* record = static_cast<Closing*>(arg);
            record->owner->closing.erase(record->pcb);
            delete record;
        }
    };
};

Stack::Stack(transport::PacketIo& tunnel, std::string local, std::string peer)
    : tunnel_(tunnel)
    , local_text_(std::move(local))
    , peer_text_(std::move(peer))
    , impl_(std::make_unique<Impl>())
{
    impl_->owner = this;
    addresses_valid_ = inet_pton(AF_INET6, local_text_.c_str(), local_addr_.data()) == 1
                       && inet_pton(AF_INET6, peer_text_.c_str(), peer_addr_.data()) == 1;
}

Stack::~Stack()
{
    stop_pump();
}

netif* Stack::network_interface()
{
    return impl_->registered && pumping_ && !stopping_ ? &impl_->nic : nullptr;
}

void Stack::attach_endpoint(const void* key, std::function<void(const std::string&)> fail)
{
    impl_->endpoints.emplace(key, std::move(fail));
}

void Stack::detach_endpoint(const void* key)
{
    impl_->endpoints.erase(key);
}

void Stack::close_tcp(tcp_pcb* pcb)
{
    const auto id = LwipRuntime::instance().close_arg_id();
    static const tcp_ext_arg_callbacks callbacks { Impl::Closing::destroyed, nullptr };
    auto record = std::make_unique<Impl::Closing>(Impl::Closing { impl_.get(), pcb });
    impl_->closing.insert(pcb);
    tcp_ext_arg_set(pcb, id, record.release());
    tcp_ext_arg_set_callbacks(pcb, id, &callbacks);
    // 端点即将销毁，先解绑其回调。扩展参数仍跟踪 PCB，直到正常关闭或 abort 释放。
    tcp_arg(pcb, nullptr);
    tcp_recv(pcb, nullptr);
    tcp_err(pcb, nullptr);
    tcp_sent(pcb, nullptr);
    if (tcp_close(pcb) != ERR_OK) {
        tcp_abort(pcb);
    }
}

bool Stack::start_pump(std::string& err)
{
    if (!addresses_valid_ || stopping_ || pumping_) {
        return err = MAA_IOS_TR("Cannot start tunnel stack: invalid address or stack already started/stopped"), false;
    }
    const auto mtu = tunnel_.mtu();
    if (mtu < 1280) {
        return err = MAA_IOS_TR("Tunnel MTU is below the IPv6 minimum"), false;
    }
    const bool registered = LwipRuntime::instance().call([&] {
        ip6_addr_t ip { };
        ip6addr_aton(local_text_.c_str(), &ip);
        if (!netif_add_noaddr(&impl_->nic, impl_.get(), Impl::init, ip6_input)) {
            return false;
        }
        impl_->registered = true;
        impl_->nic.mtu = mtu;
        netif_ip6_addr_set(&impl_->nic, 0, &ip);
        netif_ip6_addr_set_state(&impl_->nic, 0, IP6_ADDR_PREFERRED);
        netif_set_up(&impl_->nic);
        netif_set_link_up(&impl_->nic);
        return true;
    });
    if (!registered) {
        return err = MAA_IOS_TR("Failed to register lwIP tunnel interface"), false;
    }
    pumping_ = true;
    try {
        pump_ = std::thread(&Stack::pump_loop, this);
    }
    catch (const std::system_error& e) {
        err = MAA_IOS_TR("Failed to start tunnel thread: ") + std::string(e.what());
        stop_pump();
        return false;
    }
    err.clear();
    return true;
}

void Stack::fail_endpoints(const std::string& reason)
{
    LwipRuntime::instance().call([&] {
        for (auto& [key, fail] : impl_->endpoints) {
            fail(reason);
        }
    });
}

void Stack::stop_pump()
{
    if (!impl_->registered) {
        return;
    }
    if (net_debug_) {
        std::fprintf(stderr, "tunnel: local shutdown requested\n");
    }
    stopping_ = true;
    {
        std::lock_guard lock(err_mu_);
        if (pump_err_.empty()) {
            pump_err_ = MAA_IOS_TR("Tunnel stopped");
        }
    }
    // 先取消隧道等待，再通知端点失败；fd 由拥有者在线程退出后释放。
    // 此时不持有错误锁或队列锁，避免核心任务和隧道线程互相等待。
    tunnel_.shutdown();
    fail_endpoints(pump_error());
    if (pump_.joinable()) {
        pump_.join();
    }
    pumping_ = false;
    // 隧道线程已退出，不会再向 netif 提交输入；核心线程收尾后才释放出站队列。
    LwipRuntime::instance().call([&] {
        while (!impl_->closing.empty()) {
            tcp_abort(*impl_->closing.begin());
        }
        netif_set_down(&impl_->nic);
        netif_set_link_down(&impl_->nic);
        netif_remove(&impl_->nic);
        impl_->registered = false;
    });
    std::lock_guard lock(impl_->queue_mutex);
    impl_->outgoing.clear();
    impl_->queued_bytes = 0;
}

std::string Stack::pump_error() const
{
    std::lock_guard lock(err_mu_);
    return pump_err_;
}

bool Stack::enqueue(std::vector<uint8_t> packet)
{
    if (stopping_ || packet.size() < 40) {
        return false;
    }
    const uint32_t label = flow_label_;
    packet[1] = static_cast<uint8_t>((packet[1] & 0xf0) | (label >> 16));
    packet[2] = static_cast<uint8_t>(label >> 8);
    packet[3] = static_cast<uint8_t>(label);
    std::lock_guard lock(impl_->queue_mutex);
    constexpr size_t max_bytes = 4u << 20;
    // 限制待写入隧道的总字节数；慢隧道不能让核心线程持续积压包或等待 I/O。
    if (impl_->queued_bytes + packet.size() > max_bytes) {
        return false;
    }
    const auto size = packet.size();
    impl_->outgoing.push_back(std::move(packet));
    impl_->queued_bytes += size;
    return true;
}

bool Stack::send(const std::vector<uint8_t>& packet, std::string& err)
{
    if (!pumping_ || !enqueue(packet)) {
        return err = MAA_IOS_TR("Tunnel stopped or send queue full"), false;
    }
    return true;
}

void Stack::pump_loop()
{
    std::string error;
    try {
        while (!stopping_) {
            // 每轮最多发送 64 个包，持续出站时仍给入站 ACK 和媒体包留出处理机会。
            for (unsigned n = 0; n < 64 && !stopping_; ++n) {
                std::vector<uint8_t> packet;
                {
                    std::lock_guard lock(impl_->queue_mutex);
                    if (impl_->outgoing.empty()) {
                        break;
                    }
                    packet = std::move(impl_->outgoing.front());
                    impl_->outgoing.pop_front();
                    impl_->queued_bytes -= packet.size();
                }
                // 出队后释放队列锁，再进入可能阻塞的隧道写入。
                if (!tunnel_.send_ipv6(packet.data(), packet.size(), error)) {
                    break;
                }
            }
            if (!error.empty()) {
                break;
            }
            bool timed_out = false;
            if (!tunnel_.wait_readable(5, error, &timed_out)) {
                if (timed_out) {
                    continue;
                }
                break;
            }
            std::vector<uint8_t> packet;
            if (!tunnel_.recv_ipv6(packet, error)) {
                break;
            }
            if (packet.size() < 40 || packet.size() > 65535 || packet[0] >> 4 != 6) {
                continue;
            }
            // 这里只记录直接位于 IPv6 头之后的协议诊断；完整扩展头、TCP、UDP
            // 和 ICMP 处理仍交给 lwIP，诊断计数不会替代协议校验或拦截输入。
            const uint8_t protocol = packet[6];
            if ((protocol == 6 || protocol == 17) && packet.size() >= 48
                && l4_checksum(packet.data() + 8, packet.data() + 24, packet.data() + 40, packet.size() - 40, protocol) != 0) {
                ++bad_checksums_;
            }
            if (protocol == 58 && packet.size() >= 44) {
                observe_icmpv6(packet.data() + 40, packet.size() - 40);
            }
            LwipRuntime::instance().call([&] {
                if (stopping_) {
                    return;
                }
                auto* p = pbuf_alloc(PBUF_RAW, static_cast<u16_t>(packet.size()), PBUF_RAM);
                if (!p) {
                    return;
                }
                pbuf_take(p, packet.data(), static_cast<u16_t>(packet.size()));
                if (ip6_input(p, &impl_->nic) != ERR_OK) {
                    pbuf_free(p);
                }
            });
        }
    }
    catch (const std::exception& e) {
        error = MAA_IOS_TR("Tunnel processing failed: ") + std::string(e.what());
    }
    if (!stopping_) {
        {
            std::lock_guard lock(err_mu_);
            pump_err_ = error.empty() ? MAA_IOS_TR("Tunnel read failed") : error;
        }
        // 后续端点可能只报告 lwIP 已关闭；诊断模式先保留隧道的首个错误。
        if (net_debug_) {
            std::fprintf(stderr, "tunnel: pump failed: %s\n", pump_error().c_str());
        }
        stopping_ = true;
        fail_endpoints(pump_error());
    }
    pumping_ = false;
}

std::vector<uint8_t> Stack::wrap(const std::vector<uint8_t>& l4, uint8_t protocol) const
{
    std::vector<uint8_t> packet(40 + l4.size());
    packet[0] = 0x60;
    packet[4] = static_cast<uint8_t>(l4.size() >> 8);
    packet[5] = static_cast<uint8_t>(l4.size());
    packet[6] = protocol;
    packet[7] = 64;
    std::copy(local_addr_.begin(), local_addr_.end(), packet.begin() + 8);
    std::copy(peer_addr_.begin(), peer_addr_.end(), packet.begin() + 24);
    std::copy(l4.begin(), l4.end(), packet.begin() + 40);
    return packet;
}

std::string Stack::icmp_last() const
{
    std::lock_guard<std::mutex> lock(icmp_mu_);
    return icmp_last_;
}

bool Stack::send_echo_request(uint16_t ident, uint16_t seq, std::string& err)
{
    // ICMPv6 头包含 type、code 和 2 字节校验和；回显请求再附带 id、seq 和数据。
    // 计算时将校验和字段置零，IPv6 伪头的 next header 使用 58。
    std::vector<uint8_t> msg = { 128,
                                 0,
                                 0,
                                 0,
                                 static_cast<uint8_t>(ident >> 8),
                                 static_cast<uint8_t>(ident),
                                 static_cast<uint8_t>(seq >> 8),
                                 static_cast<uint8_t>(seq) };
    const std::string tag = "maa-ios-canary";
    msg.insert(msg.end(), tag.begin(), tag.end());
    const uint16_t sum = l4_checksum(local_addr_.data(), peer_addr_.data(), msg.data(), msg.size(), 58);
    msg[2] = static_cast<uint8_t>(sum >> 8);
    msg[3] = static_cast<uint8_t>(sum);
    return send(wrap(msg, 58), err);
}

/// 记录 ICMPv6 头及错误消息引用的内层 IPv6 包，供诊断对端回复及触发报文。
/// 此处不验证 ICMPv6 校验和；记录到回复只表示观察到报文，协议有效性由 lwIP 判断。
void Stack::observe_icmpv6(const uint8_t* icmp, std::size_t len)
{
    if (len < 4) {
        return;
    }
    const uint8_t type = icmp[0];
    const uint8_t code = icmp[1];
    // ICMPv6 type=1 的端口不可达为 code=4；ICMPv4 使用 code=3，不能共用码表。
    static constexpr const char* kCodes[] = { MAA_IOS_N_("no route to destination"),
                                              MAA_IOS_N_("communication administratively prohibited"),
                                              MAA_IOS_N_("beyond source address scope"),
                                              MAA_IOS_N_("address unreachable"),
                                              MAA_IOS_N_("port unreachable"),
                                              MAA_IOS_N_("source address failed ingress/egress policy"),
                                              MAA_IOS_N_("route to destination rejected") };
    std::string line = "ICMPv6 type=" + std::to_string(type) + " code=" + std::to_string(code);
    if (type == 1 && code < std::size(kCodes)) {
        line += std::string(MAA_IOS_TR("(")) + MAA_IOS_TR(kCodes[code]) + MAA_IOS_TR(")");
    }
    else if (type == 2) {
        line += MAA_IOS_TR(" (packet too big)");
    }
    else if (type == 3) {
        line += MAA_IOS_TR(" (hop limit exceeded)");
    }
    else if (type == 4) {
        line += MAA_IOS_TR(" (parameter problem)");
    }
    else if (type == 128 || type == 129) {
        line += type == 128 ? MAA_IOS_TR(" (echo request)") : MAA_IOS_TR(" (echo reply)");
    }
    // 错误消息 type=1..4 在 8 字节 ICMPv6 头之后引用触发报文：先是 40 字节
    // IPv6 头，再是 L4 数据。UDP 头前 8 字节包含端口、长度和校验和，用于
    // 将端口不可达回复与发出的 RTCP 等数据报对应起来。
    if (type <= 4 && len >= 8 + 40 + 8) {
        const uint8_t* inner = icmp + 8;
        const std::size_t rest = len - 8;
        const uint8_t inner_nh = inner[6];
        char src[64] = "?";
        char dst[64] = "?";
        inet_ntop(AF_INET6, inner + 8, src, sizeof(src));
        inet_ntop(AF_INET6, inner + 24, dst, sizeof(dst));
        line += MAA_IOS_TR(" inner nh=") + std::to_string(inner_nh) + " " + src + " -> " + dst;
        if (inner_nh == 17 && rest >= 40 + 8) {
            const uint16_t sp = get16(inner + 40);
            const uint16_t dp = get16(inner + 42);
            line += MAA_IOS_TR(" ports ") + std::to_string(sp) + "->" + std::to_string(dp);
        }
    }
    {
        std::lock_guard<std::mutex> lock(icmp_mu_);
        ++icmp_seen_;
        if (type == 129) {
            ++echo_replies_;
        }
        icmp_last_ = line;
    }
    std::fprintf(stderr, "    <- %s\n", line.c_str());
}

} // namespace maa::ios::net
