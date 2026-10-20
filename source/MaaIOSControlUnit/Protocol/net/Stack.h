#pragma once
#include "transport/Tunnel.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
struct netif;
struct tcp_pcb;

namespace maa::ios::net
{
// 一条 IPv6 包隧道对应一个 lwIP netif。隧道线程独占包 I/O，协议状态和
// netif 操作交给 LwipRuntime 核心线程；两者通过有界出站队列交换数据。
// PacketIo 必须比 Stack 活得更久，端点必须先于 Stack 析构。
// start_pump/stop_pump 由拥有者串行调用；停止后不能再次启动同一个 Stack。
class Stack
{
public:
    Stack(transport::PacketIo& tunnel, std::string local, std::string peer);
    ~Stack();

    bool addresses_ok() const { return addresses_valid_; }

    const std::string& local_text() const { return local_text_; }

    const std::string& peer_text() const { return peer_text_; }

    // 注册 netif 后启动隧道线程；停止会取消隧道等待、唤醒端点，再移除 netif。
    bool start_pump(std::string& err);
    void stop_pump();

    std::string pump_error() const;
    // 以下接口只在 lwIP 核心线程调用，供内部端点适配与集成测试使用。
    // netif 在停止期间不可用；端点失败回调不得等待应用线程或修改注册表。
    netif* network_interface();
    void attach_endpoint(const void* key, std::function<void(const std::string&)> fail);
    void detach_endpoint(const void* key);
    // 关闭 TCP 后仍可能等待 FIN/ACK；Stack 保留关闭记录，最终停止时统一清理。
    void close_tcp(tcp_pcb* pcb);

private:
    struct Impl;
    void pump_loop();
    void fail_endpoints(const std::string& reason);
    void observe_icmpv6(const uint8_t* icmp, size_t length);
    bool enqueue(std::vector<uint8_t> packet);
    transport::PacketIo& tunnel_;
    std::string local_text_, peer_text_;
    std::array<uint8_t, 16> local_addr_ { }, peer_addr_ { };
    bool addresses_valid_ = false;
    std::unique_ptr<Impl> impl_;
    std::atomic<bool> stopping_ { false }, pumping_ { false };
    mutable std::mutex err_mu_;
    std::string pump_err_;
    std::thread pump_;
};

} // namespace maa::ios::net
