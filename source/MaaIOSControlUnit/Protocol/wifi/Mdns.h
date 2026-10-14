#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace maa::ios::wifi::mdns
{

struct Endpoint
{
    /// IPv6 link-local 地址带 %接口索引，可直接交给本机连接接口。
    std::string address;
    uint16_t port = 0;
    uint32_t interface_index = 0;
    std::string interface_name;
    bool operator==(const Endpoint&) const = default;
};

/// 一条 _remotepairing._tcp.local. 广播。广播标识不是 USB UDID，TXT 也不是认证结果。
/// 同一个实例在不同接口上的记录分别关联，最后保留所有地址和真实 SRV 端口。
struct Advertisement
{
    std::string instance;
    std::string identifier;
    std::string auth_tag;                   ///< TXT authTag 原始字节，可能不是可打印字符串
    std::string target;
    std::string name;                       ///< TXT name/displayName 有值时提供，否则为空
    std::map<std::string, std::string> txt; ///< 键转 ASCII 小写，值保持原始字节
    std::vector<Endpoint> endpoints;
};

struct BrowseOptions
{
    std::chrono::milliseconds timeout { 3000 };
    /// 留空表示不取消。回调应快速返回且不抛异常；共享状态由调用方同步。
    /// 可捕获 std::stop_token 或原子标志，不要求系统 libc++ 已公开 stop_token。
    std::function<bool()> should_cancel;
};

struct BrowseResult
{
    std::vector<Advertisement> advertisements;
    bool cancelled = false;
    bool available = false; ///< 至少一个接口成功发送查询；空列表不代表扫描失败
    std::vector<std::string> warnings;
};

/// 使用各个已启用的组播网卡做有期限的查询，不配对、不连接设备服务。
/// 零超时直接返回空快照，不枚举接口或发包。取消保留已收到的快照并置 cancelled，
/// 收包等待最多 50 ms；接口或网络错误放入 warnings，不写终端。
/// 超时范围是 0..60 秒；超过范围返回 warning，不进行网络操作。
BrowseResult browse(const BrowseOptions& options = { });

/// 手机主动发现并连接本机时公布的身份；identifier/authTag 只用于发现，
/// 不代表认证结果。name/model 为 TXT 值，端口须由调用方先完成 TCP 监听。
struct HostAdvertisement
{
    /// 同时用作 DNS instance/target 的单个 ASCII label：1..63 个字母/数字/连字符，
    /// 首尾须为字母或数字；调用方为每次新的配对会话生成 fresh UUID。
    std::string instance_identifier;
    std::string name;
    std::string model;
    std::string auth_tag;
    uint16_t port = 0;
    bool advertise_ipv4 = true;
    bool advertise_ipv6 = true;
};

/// 公告固定 _remotepairing-pairable-host._tcp.local. 服务。
/// DNS instance 首个 label、target 首个 label 与 TXT identifier 使用同一个身份。
/// 单线程拥有者顺序调用；不创建接收线程或 TCP 连接，不承担配对认证。
class Advertiser
{
public:
    static std::unique_ptr<Advertiser> start(const HostAdvertisement& host, std::string& error);
    /// 最多等待 timeout_ms（0..60000）；无查询也是成功。调用方用短片段检查取消。
    /// 回应本机 PTR/SRV/TXT/A/AAAA 查询并周期刷新 TTL=120 秒的公告。
    bool poll(int timeout_ms, std::string& error);
    /// 最后尽力发送 TTL=0 goodbye，再关闭本对象拥有的全部 socket。
    ~Advertiser();
    Advertiser(const Advertiser&) = delete;
    Advertiser& operator=(const Advertiser&) = delete;

private:
    struct Impl;
    explicit Advertiser(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl_;
};

namespace detail
{

/// 同一接口的报文输入；DNS instance/target 首 label 必须与 host identifier 一致。
/// 公开仅用于离线验证编码/查询分派，不选择系统 endpoint。
struct HostRecords
{
    HostAdvertisement host;
    std::string service_instance;
    std::string target;
    std::vector<std::string> addresses;
};

struct HostReply
{
    std::vector<uint8_t> packet;
    bool unicast = false;
};

std::vector<uint8_t> host_announcement(const HostRecords& records, uint32_t ttl, std::string& error);
/// legacy 为源端口不是 5353 的查询；回应保留 query ID/问题并限制 TTL=10。
std::vector<HostReply> host_query_reply(const HostRecords& records, std::span<const uint8_t> query, bool legacy, std::string& error);

/// 扫描器的报文聚合器；单独开放这条内部边界，方便离线验证压缩名称和跨报文关联。
/// 不缓存到下一次扫描。now 参数只用于本次扫描的 TTL 判断。
class RecordCache
{
public:
    using Clock = std::chrono::steady_clock;
    RecordCache();
    ~RecordCache();
    RecordCache(const RecordCache&) = delete;
    RecordCache& operator=(const RecordCache&) = delete;
    /// interface_index 是同一网卡的记录关联索引及 IPv4 索引；Windows 的 IPv6
    /// 索引另传 ipv6_scope_index，用于 AAAA 候选和 link-local scope。省略时沿用
    /// interface_index；显式为 0 表示该网卡没有可用的 IPv6 索引。
    bool add_packet(
        uint32_t interface_index,
        std::string_view interface_name,
        std::span<const uint8_t> packet,
        Clock::time_point now = Clock::now(),
        std::optional<uint32_t> ipv6_scope_index = std::nullopt);
    std::vector<Advertisement> snapshot(Clock::time_point now = Clock::now()) const;
    /// 尚缺的 SRV/TXT 或主机 A/AAAA 查询，按接口隔离。
    std::vector<std::pair<uint16_t, std::string>> queries(uint32_t interface_index, Clock::time_point now = Clock::now()) const;
    const std::vector<std::string>& warnings() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace detail
} // namespace maa::ios::wifi::mdns
