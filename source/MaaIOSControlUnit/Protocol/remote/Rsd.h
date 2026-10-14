#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

#include "net/Stack.h"
#include "net/TcpStream.h"
#include "remote/RemoteXpc.h"
#include "transport/IoBudget.h"
#include "xpc/XpcValue.h"

namespace maa::ios::remote
{

/// RSD peer_info.Services 中的服务条目；name 来自目录字典的键。
struct ServiceInfo
{
    std::string name;
    /// Port 接受 0..65535 的完整十进制字符串、Int64 或 UInt64。
    /// 缺失、0 或非法值均保留为 0，表示没有可连接端口，不删除目录中的服务项。
    uint16_t port = 0;
    /// Properties.UsesRemoteXPC 决定是否在 TCP 上建立 RemoteXPC 通道。
    bool uses_remote_xpc = false;
    /// 保留 Properties.EncryptSocketData 的声明；当前 open 不据此建立 TLS。
    bool encrypt_socket_data = false;
    /// 顶层 Entitlement 和 Properties.Features 供能力查询及诊断使用。
    std::string entitlement;
    std::vector<std::string> features;
};

/// 解析 RSD 的 Services 字典，不进行网络 I/O。保持字典条目顺序，跳过非字典条目；
/// 无效端口仍保留服务的属性和能力信息。非字典输入返回空列表，Rsd::open 另行报告目录错误。
[[nodiscard]] std::vector<ServiceInfo> parse_service_directory(const xpc::Value& services);

/// 区分成功、能力/业务错误和连接/传输失败，供调用方决定后续处理。
/// TransportError 不表示设备一定未执行请求，重试策略由调用方决定。
enum class CallResult
{
    Ok,
    DeviceError,
    TransportError
};

/// 持有独立 TCP 服务连接。UsesRemoteXPC=true 时建立 HTTP/2 + RemoteXPC 通道；
/// 其它服务由调用方通过 tcp() 处理原始协议。Stack 必须比连接活得更久。
/// 同一连接的请求和读消息由调用方串行调度，常驻订阅应独占连接。
class ServiceConnection
{
public:
    ServiceConnection() = default;

    /// 端口为 0 时在创建 TCP 连接前返回 nullptr，并在 err 中报告服务名与有效范围。
    /// cancel 从建连前生效，返回后仍可中断该连接的读写；不能撤销已被设备执行的请求。
    static std::unique_ptr<ServiceConnection>
        open(net::Stack& stack, const ServiceInfo& service, std::string& err, bool verbose = false, std::stop_token cancel = { });

    /// 将 input 放入 CoreDevice.input，成功时读取 CoreDevice.output。
    /// 设备错误或缺少预期输出返回 DeviceError，并在 err 中保留诊断。
    CallResult invoke(
        std::string_view feature_identifier,
        std::string_view action_identifier,
        const xpc::Value& input,
        xpc::Value& output,
        int timeout_ms,
        std::string& err);

    /// 一次 RemoteXPC 往返，不添加 CoreDevice 请求外壳。
    bool call(const xpc::Value& request, xpc::Value& reply, int timeout_ms, std::string& err);

    /// 不设置 WANTING_REPLY，只发送 RemoteXPC 请求，不等待业务回复。
    bool send_only(const xpc::Value& request, std::string& err);

    /// 单独等待下一条 RemoteXPC 消息，区分 Message、Timeout 和 Broken。
    /// 配合 send_only 处理一次发送对应多条回复或没有业务回复的协议。
    Channel::Wait wait_message(xpc::Value& out, int timeout_ms, std::string& err);

    /// 订阅后逐批消费 element，直到 finishStreaming 或 on_element 返回 false。
    /// input 由调用方包装 actualInput 和 streamProxy.sideChannel；回复状态为
    /// pushing:{elements:[...]}、finishStreaming 或 receivedError。
    /// timeout_ms 是每次 next_batch 的等待上限；Idle 在此返回 TransportError。
    /// 回调返回 false 只结束本地消费循环，不额外发送取消消息。
    CallResult stream(
        std::string_view feature_identifier,
        std::string_view action_identifier,
        const xpc::Value& input,
        const std::function<bool(const xpc::Value&)>& on_element,
        int timeout_ms,
        std::string& err);

    /// 发送流式订阅请求，记录 feature 名，后续由 next_batch 分步取回推送。
    /// 常驻订阅可在相邻批次之间检查停止标志，避免一次 stream 循环长期占用线程。
    bool subscribe(std::string_view feature_identifier, std::string_view action_identifier, const xpc::Value& input, std::string& err);

    /// Batch 表示已收到推送，Finished 表示服务结束；Idle 仅表示本轮等待超时。
    /// DeviceError/Broken 分别表示业务错误和连接/协议失败。
    enum class StreamEvent
    {
        Batch,
        Finished,
        Idle,
        DeviceError,
        Broken
    };

    /// 每次先清空 elements；一条 pushing 可带多个元素，空批次也返回 Batch。
    StreamEvent next_batch(std::vector<xpc::Value>& elements, int timeout_ms, std::string& err);

    [[nodiscard]] net::TcpStream& tcp() { return *tcp_; }

    [[nodiscard]] bool is_xpc() const { return channel_ != nullptr; }

    [[nodiscard]] Channel* channel() { return channel_.get(); }

    /// 处理 HTTP/2 PING、WINDOW_UPDATE 等控制帧；读超时仍返回 true。
    /// false 表示连接或协议处理失败，不表示本轮没有业务更新。
    bool service(int timeout_ms, std::string& err);

private:
    /// channel_ 引用 tcp_，必须在 tcp_ 之后声明，保证先释放 Channel。
    std::unique_ptr<net::TcpStream> tcp_;
    std::unique_ptr<Channel> channel_;
    /// 当前订阅的 feature 名，用于缺少请求上下文的推送错误诊断。
    std::string subscribed_feature_;
    bool cancelled(std::string& err) const;

    struct CloseOnStop
    {
        net::TcpStream* tcp;

        void operator()() const { tcp->close(); }
    };

    std::stop_token cancel_;
    /// 回调只能关闭端点，不能访问 Channel。最后声明以先解除注册、等待正在执行的
    /// 回调结束，再析构其引用的 Channel/TCP；连接本身仍由请求线程独占。
    std::optional<std::stop_callback<CloseOnStop>> cancellation_;
};

/// 借用 Stack，持有 RSD 控制通道及首次握手取得的服务目录快照。
class Rsd
{
public:
    /// 在隧道的 RSD 端口上建控制通道并读目录。
    static std::optional<Rsd>
        open(net::Stack& stack, transport::PacketTunnel& tunnel, const PeerIdentity& identity, std::string& err, bool verbose = false);

    /// 初始 TCP、HTTP/2 和目录读取共用调用方的绝对预算。取消或预算到期
    /// 只关闭这个控制端点；成功交付后解除监视，不影响会话后续的服务连接。
    static std::optional<Rsd> open_bounded(
        net::Stack& stack,
        transport::PacketTunnel& tunnel,
        const PeerIdentity& identity,
        const transport::IoBudget& budget,
        std::string& err,
        bool verbose = false);

    /// 目录里没有该项时返回 nullopt。
    [[nodiscard]] std::optional<ServiceInfo> service(std::string_view name) const;
    [[nodiscard]] bool has_service(std::string_view name) const;
    /// 查询首次目录中服务声明的 feature 集合，不额外访问设备。
    [[nodiscard]] bool supports(std::string_view service_name, std::string_view feature) const;

    [[nodiscard]] const std::vector<ServiceInfo>& services() const { return services_; }

    /// 缺少服务时报告目录总数及 CoreDevice 服务名，供调用方诊断能力差异。
    static std::string missing_service_message(std::string_view name, const std::vector<ServiceInfo>& seen);

    /// 对包含 requires iOS 的设备错误补充已记录的版本限制和可选兜底功能提示。
    static std::string remote_control_version_hint(std::string_view device_detail);

    /// 返回原始 Services 条目的借用指针，保留 ServiceInfo 未解析的字段。
    /// 缺失时为 nullptr，返回值不能超过 Rsd 的生命周期。
    [[nodiscard]] const xpc::Value* service_entry(std::string_view name) const;

    std::unique_ptr<ServiceConnection>
        connect_service(std::string_view name, std::string& err, bool verbose = false, std::stop_token cancel = { });

    /// 返回 peer_info.Properties 的借用指针；缺失时为 nullptr，日志脱敏由调用方负责。
    [[nodiscard]] const xpc::Value* properties() const;

    [[nodiscard]] net::Stack& stack() { return *stack_; }

    [[nodiscard]] const PeerIdentity& identity() const { return identity_; }

    /// 公开构造供 optional::emplace 使用；尚未建立控制通道，可用目录应通过 open 获取。
    Rsd(net::Stack& stack, PeerIdentity identity)
        : stack_(&stack)
        , identity_(std::move(identity))
    {
    }

private:
    /// 不持有 Stack 所有权；其生命周期必须覆盖控制通道和所有服务连接。
    net::Stack* stack_ = nullptr;
    PeerIdentity identity_;
    /// control_ 引用 tcp_，必须声明在 tcp_ 之后，保证按逆序先释放 Channel。
    std::unique_ptr<net::TcpStream> tcp_;
    std::unique_ptr<Channel> control_;
    std::vector<ServiceInfo> services_;
};

/// 生成 UUID v4 文本，用于默认请求标识；RSD PeerIdentity 使用独立的稳定身份。
[[nodiscard]] std::string random_uuid_text();

/// 组装 CoreDevice 那条统一的请求外壳。
[[nodiscard]] xpc::Value
    core_device_request(std::string_view feature_identifier, std::string_view action_identifier, const xpc::Value& input);

} // namespace maa::ios::remote
