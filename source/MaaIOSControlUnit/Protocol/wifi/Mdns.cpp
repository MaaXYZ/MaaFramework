#include "wifi/Mdns.h"

#include "transport/SocketPlatform.h"
#include <mdns.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <exception>
#include <limits>
#include <set>
#include <tuple>
#include <utility>

#ifdef _WIN32
#include <iphlpapi.h>
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#endif

namespace maa::ios::wifi::mdns
{
namespace
{

constexpr std::string_view kService = "_remotepairing._tcp.local.";
constexpr size_t kMaxPacket = 16384;
constexpr size_t kMaxRecords = 512;
constexpr size_t kMaxInterfaces = 64;
constexpr size_t kMaxNames = 256;
constexpr size_t kMaxAddresses = 32;
constexpr size_t kMaxSrv = 8;
constexpr size_t kMaxEndpoints = 256;
constexpr size_t kMaxPackets = 2048;

std::string lower(std::string_view value)
{
    std::string result(value);
    for (char& ch : result) {
        if (ch >= 'A' && ch <= 'Z') {
            ch += 'a' - 'A';
        }
    }
    return result;
}

bool service_instance(std::string_view name)
{
    const std::string normalized = lower(name);
    return normalized.size() > kService.size() && normalized.ends_with(kService)
           && normalized[normalized.size() - kService.size() - 1] == '.';
}

std::string extract_name(const void* data, size_t size, size_t offset)
{
    const size_t original = offset;
    std::array<char, 256> output { };
    const auto name = mdns_string_extract(data, size, &offset, output.data(), output.size());
    if (offset == original || !name.length || name.length >= output.size()) {
        return { };
    }
    std::string result(name.str, name.length);
    if (result.find('\0') != std::string::npos || result.back() != '.') {
        return { };
    }
    return result;
}

template <typename T>
struct Timed
{
    T value;
    detail::RecordCache::Clock::time_point expires;
};

struct Service
{
    std::string instance;
    detail::RecordCache::Clock::time_point ptr_expires { };

    struct Srv
    {
        std::string target;
        uint16_t port = 0;
        bool operator==(const Srv&) const = default;
    };

    std::vector<Timed<Srv>> srv;
    std::map<std::string, std::string> txt;
    detail::RecordCache::Clock::time_point txt_expires { };
};

struct InterfaceRecords
{
    std::string name;
    uint32_t ipv6_index = 0;
    std::map<std::string, Service> services;
    std::map<std::string, std::vector<Timed<std::string>>> hosts;
};

template <typename T>
bool update(std::vector<Timed<T>>& records, const T& value, uint32_t ttl, detail::RecordCache::Clock::time_point now, size_t limit)
{
    std::erase_if(records, [&](const Timed<T>& record) { return record.expires <= now || (ttl == 0 && record.value == value); });
    if (!ttl) {
        return true;
    }
    const auto expires = now + std::chrono::seconds(ttl);
    for (auto& record : records) {
        if (record.value == value) {
            record.expires = expires;
            return true;
        }
    }
    if (records.size() >= limit) {
        return false;
    }
    records.push_back({ value, expires });
    return true;
}

void warn(std::vector<std::string>& warnings, std::string message)
{
    if (warnings.size() < 32 && std::find(warnings.begin(), warnings.end(), message) == warnings.end()) {
        warnings.push_back(std::move(message));
    }
}

} // namespace

struct detail::RecordCache::Impl
{
    std::map<uint32_t, InterfaceRecords> interfaces;
    std::vector<std::string> warnings;
    size_t service_count = 0;
    size_t host_count = 0;

    struct Context
    {
        Impl& cache;
        InterfaceRecords& iface;
        uint32_t ipv6_scope_index;
        Clock::time_point now;
        bool valid = true;
    };

    static int record(
        int,
        const sockaddr*,
        size_t,
        mdns_entry_type_t,
        uint16_t,
        uint16_t type,
        uint16_t record_class,
        uint32_t ttl,
        const void* data,
        size_t size,
        size_t name_offset,
        size_t,
        size_t offset,
        size_t length,
        void* user)
    {
        auto& ctx = *static_cast<Context*>(user);
        if ((record_class & 0x7fff) != MDNS_CLASS_IN) {
            return 0;
        }
        const std::string name = extract_name(data, size, name_offset);
        if (name.empty()) {
            ctx.valid = false;
            return 0;
        }
        const auto key = lower(name);
        auto service = [&](std::string_view instance) -> Service* {
            if (!service_instance(instance)) {
                return nullptr;
            }
            const auto normalized = lower(instance);
            auto found = ctx.iface.services.find(normalized);
            if (found != ctx.iface.services.end()) {
                return &found->second;
            }
            if (ctx.cache.service_count >= kMaxNames) {
                warn(ctx.cache.warnings, "mDNS service limit reached");
                return nullptr;
            }
            ++ctx.cache.service_count;
            Service entry;
            entry.instance = instance;
            return &ctx.iface.services.emplace(normalized, std::move(entry)).first->second;
        };
        if (type == MDNS_RECORDTYPE_PTR && key == kService) {
            const auto instance = extract_name(data, size, offset);
            if (instance.empty()) {
                ctx.valid = false;
                return 0;
            }
            if (auto* entry = service(instance)) {
                entry->ptr_expires = ctx.now + std::chrono::seconds(ttl);
            }
        }
        else if (type == MDNS_RECORDTYPE_SRV) {
            if (auto* entry = service(name)) {
                std::array<char, 256> buffer { };
                const auto srv = mdns_record_parse_srv(data, size, offset, length, buffer.data(), buffer.size());
                const auto target = length >= 8 ? extract_name(data, size, offset + 6) : std::string { };
                if (target.empty()) {
                    ctx.valid = false;
                    return 0;
                }
                if (srv.port) {
                    if (!update(entry->srv, Service::Srv { target, srv.port }, ttl, ctx.now, kMaxSrv)) {
                        warn(ctx.cache.warnings, "mDNS SRV candidate limit reached");
                    }
                }
            }
        }
        else if (type == MDNS_RECORDTYPE_TXT) {
            if (auto* entry = service(name)) {
                if (length > 4096) {
                    warn(ctx.cache.warnings, "mDNS TXT record limit reached");
                    return 0;
                }
                std::array<mdns_record_txt_t, 128> fields { };
                const auto count = mdns_record_parse_txt(data, size, offset, length, fields.data(), fields.size());
                std::map<std::string, std::string> txt;
                for (size_t i = 0; i < count; ++i) {
                    const auto field_key = lower({ fields[i].key.str, fields[i].key.length });
                    // RFC 6763 要求忽略同名后续键；保留第一项，防止广播匹配与显示使用不同值。
                    txt.try_emplace(
                        field_key,
                        fields[i].value.str ? std::string(fields[i].value.str, fields[i].value.length) : std::string { });
                }
                entry->txt = std::move(txt);
                entry->txt_expires = ctx.now + std::chrono::seconds(ttl);
            }
        }
        else if (type == MDNS_RECORDTYPE_A || type == MDNS_RECORDTYPE_AAAA) {
            std::string address;
            std::array<char, INET6_ADDRSTRLEN> buffer { };
            if (type == MDNS_RECORDTYPE_A && length == 4) {
                sockaddr_in parsed { };
                mdns_record_parse_a(data, size, offset, length, &parsed);
                const uint32_t ipv4 = ntohl(parsed.sin_addr.s_addr);
                if (ipv4 == 0 || ipv4 == 0xffffffff || (ipv4 >> 24) == 127 || (ipv4 >> 28) >= 14) {
                    return 0;
                }
                if (inet_ntop(AF_INET, &parsed.sin_addr, buffer.data(), buffer.size())) {
                    address = buffer.data();
                }
            }
            else if (type == MDNS_RECORDTYPE_AAAA && length == 16) {
                sockaddr_in6 parsed { };
                mdns_record_parse_aaaa(data, size, offset, length, &parsed);
                if (IN6_IS_ADDR_UNSPECIFIED(&parsed.sin6_addr) || IN6_IS_ADDR_LOOPBACK(&parsed.sin6_addr)
                    || IN6_IS_ADDR_MULTICAST(&parsed.sin6_addr)) {
                    return 0;
                }
                if (inet_ntop(AF_INET6, &parsed.sin6_addr, buffer.data(), buffer.size())) {
                    address = buffer.data();
                    if (IN6_IS_ADDR_LINKLOCAL(&parsed.sin6_addr)) {
                        if (!ctx.ipv6_scope_index) {
                            return 0;
                        }
                        address += '%' + std::to_string(ctx.ipv6_scope_index);
                    }
                }
            }
            else {
                ctx.valid = false;
                return 0;
            }
            if (!address.empty()) {
                if (!ctx.iface.hosts.contains(key) && ctx.cache.host_count >= kMaxNames) {
                    warn(ctx.cache.warnings, "mDNS host limit reached");
                    return 0;
                }
                if (!ctx.iface.hosts.contains(key)) {
                    ++ctx.cache.host_count;
                }
                if (!update(ctx.iface.hosts[key], address, ttl, ctx.now, kMaxAddresses)) {
                    warn(ctx.cache.warnings, "mDNS address candidate limit reached");
                }
            }
        }
        return 0;
    }
};

detail::RecordCache::RecordCache()
    : impl_(std::make_unique<Impl>())
{
}

detail::RecordCache::~RecordCache() = default;

bool detail::RecordCache::add_packet(
    uint32_t index,
    std::string_view interface_name,
    std::span<const uint8_t> packet,
    Clock::time_point now,
    std::optional<uint32_t> ipv6_scope_index)
{
    if (packet.size() < sizeof(mdns_header_t) || packet.size() > kMaxPacket) {
        return false;
    }
    const auto flags = mdns_ntohs(packet.data() + 2);
    // 只接受成功的普通 DNS 回答；截断报文无法提供可信的关联快照。
    if ((flags & 0x8000) == 0 || (flags & 0x7a0f) != 0) {
        return false;
    }
    const size_t questions = mdns_ntohs(packet.data() + 4);
    const size_t answers = mdns_ntohs(packet.data() + 6);
    const size_t authority = mdns_ntohs(packet.data() + 8);
    const size_t additional = mdns_ntohs(packet.data() + 10);
    const size_t records = answers + authority + additional;
    if (questions + records > kMaxRecords) {
        return false;
    }
    size_t offset = sizeof(mdns_header_t);
    for (size_t i = 0; i < questions; ++i) {
        if (!mdns_string_skip(packet.data(), packet.size(), &offset) || offset > packet.size() || packet.size() - offset < 4) {
            return false;
        }
        offset += 4;
    }
    const size_t records_offset = offset;
    // 库的 record parser 允许返回部分记录。先校验整个报文的长度边界，避免把截断报文
    // 的前半段保存到缓存；名称压缩和 RDATA 解码仍全部交给 mdns.h。
    for (size_t i = 0; i < records; ++i) {
        const size_t name_offset = offset;
        if (!mdns_string_skip(packet.data(), packet.size(), &offset) || offset > packet.size() || packet.size() - offset < 10) {
            return false;
        }
        if (extract_name(packet.data(), packet.size(), name_offset).empty()) {
            return false;
        }
        const uint16_t type = mdns_ntohs(packet.data() + offset);
        const size_t length = mdns_ntohs(packet.data() + offset + 8);
        offset += 10;
        if (length > packet.size() - offset) {
            return false;
        }
        if (type == MDNS_RECORDTYPE_PTR || type == MDNS_RECORDTYPE_SRV) {
            if (length < (type == MDNS_RECORDTYPE_SRV ? 8U : 2U)) {
                return false;
            }
            size_t name_end = offset + (type == MDNS_RECORDTYPE_SRV ? 6 : 0);
            if (extract_name(packet.data(), packet.size(), name_end).empty() || !mdns_string_skip(packet.data(), packet.size(), &name_end)
                || name_end != offset + length) {
                return false;
            }
        }
        else if ((type == MDNS_RECORDTYPE_A && length != 4) || (type == MDNS_RECORDTYPE_AAAA && length != 16)) {
            return false;
        }
        else if (type == MDNS_RECORDTYPE_TXT) {
            size_t text = offset;
            while (text < offset + length) {
                const size_t field_length = packet[text++];
                if (field_length > offset + length - text) {
                    return false;
                }
                text += field_length;
            }
        }
        offset += length;
    }
    if (!impl_->interfaces.contains(index) && impl_->interfaces.size() >= kMaxInterfaces) {
        warn(impl_->warnings, "mDNS interface limit reached");
        return false;
    }
    auto& iface = impl_->interfaces[index];
    iface.name = interface_name;
    iface.ipv6_index = ipv6_scope_index.value_or(index);
    Impl::Context ctx { *impl_, iface, iface.ipv6_index, now };
    offset = records_offset;
    mdns_records_parse(-1, nullptr, 0, packet.data(), packet.size(), &offset, MDNS_ENTRYTYPE_ANSWER, 0, records, &Impl::record, &ctx);
    return ctx.valid;
}

std::vector<Advertisement> detail::RecordCache::snapshot(Clock::time_point now) const
{
    std::vector<Advertisement> result;
    for (const auto& [index, iface] : impl_->interfaces) {
        for (const auto& [key, service] : iface.services) {
            if (service.ptr_expires <= now) {
                continue;
            }
            Advertisement ad;
            ad.instance = service.instance;
            if (service.txt_expires > now) {
                ad.txt = service.txt;
            }
            auto field = [&](std::string_view key) {
                auto found = ad.txt.find(std::string(key));
                return found == ad.txt.end() ? std::string { } : found->second;
            };
            ad.identifier = field("identifier");
            ad.auth_tag = field("authtag");
            ad.name = field("name");
            if (ad.name.empty()) {
                ad.name = field("displayname");
            }
            for (const auto& srv : service.srv) {
                if (srv.expires <= now) {
                    continue;
                }
                if (ad.target.empty()) {
                    ad.target = srv.value.target;
                }
                auto found = iface.hosts.find(lower(srv.value.target));
                if (found == iface.hosts.end()) {
                    continue;
                }
                for (const auto& address : found->second) {
                    if (address.expires > now && ad.endpoints.size() < kMaxEndpoints) {
                        const auto address_index = address.value.find(':') == std::string::npos ? index : iface.ipv6_index;
                        if (address_index) {
                            ad.endpoints.push_back({ address.value, srv.value.port, address_index, iface.name });
                        }
                    }
                }
            }
            // 同一接口可能同时用 IPv4/IPv6 查询得到相同实例；跨接口只在 TXT 相同的情况下
            // 合并，不能将不同广播的 authTag 或地址拼成一台设备。
            auto found = std::find_if(result.begin(), result.end(), [&](const Advertisement& existing) {
                return lower(existing.instance) == key && existing.txt == ad.txt;
            });
            if (found == result.end()) {
                result.push_back(std::move(ad));
            }
            else {
                const size_t count = std::min(ad.endpoints.size(), kMaxEndpoints - found->endpoints.size());
                found->endpoints.insert(
                    found->endpoints.end(),
                    ad.endpoints.begin(),
                    ad.endpoints.begin() + static_cast<std::ptrdiff_t>(count));
                if (count < ad.endpoints.size()) {
                    warn(impl_->warnings, "mDNS endpoint limit reached");
                }
            }
        }
    }
    for (auto& ad : result) {
        std::sort(ad.endpoints.begin(), ad.endpoints.end(), [](const auto& a, const auto& b) {
            return std::tie(a.interface_index, a.address, a.port) < std::tie(b.interface_index, b.address, b.port);
        });
        ad.endpoints.erase(std::unique(ad.endpoints.begin(), ad.endpoints.end()), ad.endpoints.end());
    }
    std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.instance < b.instance; });
    return result;
}

std::vector<std::pair<uint16_t, std::string>> detail::RecordCache::queries(uint32_t index, Clock::time_point now) const
{
    std::set<std::pair<uint16_t, std::string>> queries;
    auto found = impl_->interfaces.find(index);
    if (found == impl_->interfaces.end()) {
        return { };
    }
    for (const auto& [key, service] : found->second.services) {
        if (service.ptr_expires <= now) {
            continue;
        }
        if (service.txt_expires <= now) {
            queries.emplace(MDNS_RECORDTYPE_TXT, service.instance);
        }
        bool has_srv = false;
        for (const auto& srv : service.srv) {
            if (srv.expires <= now) {
                continue;
            }
            has_srv = true;
            bool v4 = false, v6 = false;
            auto host = found->second.hosts.find(lower(srv.value.target));
            if (host != found->second.hosts.end()) {
                for (const auto& address : host->second) {
                    if (address.expires <= now) {
                        continue;
                    }
                    (address.value.find(':') == std::string::npos ? v4 : v6) = true;
                }
            }
            if (!v4) {
                queries.emplace(MDNS_RECORDTYPE_A, srv.value.target);
            }
            if (!v6) {
                queries.emplace(MDNS_RECORDTYPE_AAAA, srv.value.target);
            }
        }
        if (!has_srv) {
            queries.emplace(MDNS_RECORDTYPE_SRV, service.instance);
        }
    }
    return { queries.begin(), queries.end() };
}

const std::vector<std::string>& detail::RecordCache::warnings() const
{
    return impl_->warnings;
}

namespace
{

struct InterfaceAddress
{
    uint32_t index = 0;
    uint32_t cache_index = 0;
    uint32_t ipv6_index = 0;
    std::string name;
    sockaddr_storage local { };
};

std::vector<InterfaceAddress> interfaces(std::vector<std::string>& warnings)
{
    std::vector<InterfaceAddress> result;
    std::set<uint32_t> selected;
    auto append = [&](uint32_t index, uint32_t cache_index, uint32_t ipv6_index, std::string_view name, const sockaddr* address) {
        if (!index || (address->sa_family != AF_INET && address->sa_family != AF_INET6)) {
            return;
        }
        if (!selected.contains(cache_index) && selected.size() >= kMaxInterfaces) {
            warn(warnings, "mDNS interface limit reached");
            return;
        }
        if (address->sa_family == AF_INET6) {
            const auto& ipv6 = reinterpret_cast<const sockaddr_in6*>(address)->sin6_addr;
            if (IN6_IS_ADDR_UNSPECIFIED(&ipv6) || IN6_IS_ADDR_LOOPBACK(&ipv6) || IN6_IS_ADDR_MULTICAST(&ipv6)) {
                return;
            }
        }
        InterfaceAddress entry { index, cache_index, ipv6_index, std::string(name) };
        std::memcpy(&entry.local, address, address->sa_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6));
        if (address->sa_family == AF_INET) {
            reinterpret_cast<sockaddr_in*>(&entry.local)->sin_port = 0;
        }
        else {
            auto* local = reinterpret_cast<sockaddr_in6*>(&entry.local);
            local->sin6_port = 0;
            if (IN6_IS_ADDR_LINKLOCAL(&local->sin6_addr)) {
                local->sin6_scope_id = index;
            }
        }
        // 一个接口各开一个 IPv4/IPv6 socket；IPv6 优先使用带 scope 的 link-local 地址。
        auto found = std::find_if(result.begin(), result.end(), [&](const auto& item) {
            return item.index == index && item.local.ss_family == address->sa_family;
        });
        if (found == result.end()) {
            selected.insert(cache_index);
            result.push_back(std::move(entry));
        }
        else if (address->sa_family == AF_INET6 && IN6_IS_ADDR_LINKLOCAL(&reinterpret_cast<const sockaddr_in6*>(address)->sin6_addr)) {
            *found = std::move(entry);
        }
    };
#ifdef _WIN32
    ULONG bytes = 16384;
    std::vector<uint8_t> buffer(bytes);
    ULONG status = ERROR_BUFFER_OVERFLOW;
    for (int attempt = 0; attempt < 3 && status == ERROR_BUFFER_OVERFLOW; ++attempt) {
        buffer.resize(bytes);
        status = GetAdaptersAddresses(
            AF_UNSPEC,
            GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER,
            nullptr,
            reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),
            &bytes);
        if (bytes > 1024 * 1024) {
            break;
        }
    }
    if (status != NO_ERROR) {
        warn(warnings, "GetAdaptersAddresses: " + std::to_string(status));
        return result;
    }
    for (auto* adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()); adapter; adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp || adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK
            || (adapter->Flags & IP_ADAPTER_NO_MULTICAST)) {
            continue;
        }
        for (auto* address = adapter->FirstUnicastAddress; address; address = address->Next) {
            if (address->DadState == IpDadStatePreferred || address->DadState == IpDadStateDeprecated) {
                const auto family = address->Address.lpSockaddr->sa_family;
                const uint32_t index = family == AF_INET ? adapter->IfIndex : adapter->Ipv6IfIndex;
                // 同一适配器的两个协议族索引不保证相同。cache_index 只用于关联记录，
                // socket 的组播接口、link-local scope 和返回的候选仍各用正确的族索引。
                const uint32_t cache_index = adapter->IfIndex ? adapter->IfIndex : adapter->Ipv6IfIndex;
                append(
                    index,
                    cache_index,
                    adapter->Ipv6IfIndex,
                    adapter->AdapterName ? adapter->AdapterName : "",
                    address->Address.lpSockaddr);
            }
        }
    }
#else
    ifaddrs* raw = nullptr;
    if (getifaddrs(&raw) != 0) {
        warn(warnings, "getifaddrs: " + transport::socket_error_message());
        return result;
    }
    std::unique_ptr<ifaddrs, decltype(&freeifaddrs)> addresses(raw, freeifaddrs);
    for (const auto* address = raw; address; address = address->ifa_next) {
        if (!address->ifa_addr || !(address->ifa_flags & IFF_UP) || !(address->ifa_flags & IFF_MULTICAST)
            || (address->ifa_flags & IFF_LOOPBACK)) {
            continue;
        }
        const uint32_t index = if_nametoindex(address->ifa_name);
        append(index, index, index, address->ifa_name, address->ifa_addr);
    }
#endif
    return result;
}

struct QuerySocket
{
    transport::NativeSocket fd = transport::kInvalidSocket;
    InterfaceAddress iface;
    std::map<std::pair<uint16_t, std::string>, detail::RecordCache::Clock::time_point> sent;

    ~QuerySocket()
    {
        if (fd != transport::kInvalidSocket) {
            transport::close_socket(fd);
        }
    }

    QuerySocket() = default;
    QuerySocket(const QuerySocket&) = delete;
    QuerySocket& operator=(const QuerySocket&) = delete;
};

std::unique_ptr<QuerySocket> open_socket(const InterfaceAddress& iface, std::vector<std::string>& warnings, bool responder = false)
{
    auto result = std::make_unique<QuerySocket>();
    result->iface = iface;
    const int family = iface.local.ss_family;
    result->fd = socket(family, SOCK_DGRAM, IPPROTO_UDP);
    auto fail = [&]() {
        warn(
            warnings,
            std::string("mDNS interface ") + iface.name + (family == AF_INET ? " IPv4: " : " IPv6: ") + transport::socket_error_message());
        return std::unique_ptr<QuerySocket> { };
    };
    if (result->fd == transport::kInvalidSocket) {
        return fail();
    }
    sockaddr_storage binding = iface.local;
    if (responder) {
        const int reuse = 1;
        if (setsockopt(result->fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse))) {
            return fail();
        }
#if defined(SO_REUSEPORT) && !defined(_WIN32)
        if (setsockopt(result->fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse))) {
            return fail();
        }
#endif
        binding = { };
        if (family == AF_INET) {
            auto* address = reinterpret_cast<sockaddr_in*>(&binding);
            address->sin_family = AF_INET;
            address->sin_port = htons(MDNS_PORT);
        }
        else {
            auto* address = reinterpret_cast<sockaddr_in6*>(&binding);
            address->sin6_family = AF_INET6;
            address->sin6_port = htons(MDNS_PORT);
            if (setsockopt(result->fd, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char*>(&reuse), sizeof(reuse))) {
                return fail();
            }
        }
    }
    if (bind(result->fd, reinterpret_cast<const sockaddr*>(&binding), family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6))
        != 0) {
        return fail();
    }
    if (family == AF_INET) {
        const auto& address = reinterpret_cast<const sockaddr_in*>(&iface.local)->sin_addr;
        const int ttl = 255;
        if (setsockopt(result->fd, IPPROTO_IP, IP_MULTICAST_IF, reinterpret_cast<const char*>(&address), sizeof(address))
            || setsockopt(result->fd, IPPROTO_IP, IP_MULTICAST_TTL, reinterpret_cast<const char*>(&ttl), sizeof(ttl))) {
            return fail();
        }
    }
    else {
        const int hops = 255;
        if (setsockopt(result->fd, IPPROTO_IPV6, IPV6_MULTICAST_IF, reinterpret_cast<const char*>(&iface.index), sizeof(iface.index))
            || setsockopt(result->fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, reinterpret_cast<const char*>(&hops), sizeof(hops))) {
            return fail();
        }
    }
    if (responder) {
        if (family == AF_INET) {
            ip_mreq membership { };
            inet_pton(AF_INET, "224.0.0.251", &membership.imr_multiaddr);
            membership.imr_interface = reinterpret_cast<const sockaddr_in*>(&iface.local)->sin_addr;
            if (setsockopt(result->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char*>(&membership), sizeof(membership))) {
                return fail();
            }
#ifdef IP_MULTICAST_ALL
            // Linux 默认也交付其他 socket 的组播 membership；保持接口记录隔离。
            const int own_membership = 0;
            if (setsockopt(result->fd, IPPROTO_IP, IP_MULTICAST_ALL, &own_membership, sizeof(own_membership))) {
                return fail();
            }
#endif
        }
        else {
            ipv6_mreq membership { };
            inet_pton(AF_INET6, "ff02::fb", &membership.ipv6mr_multiaddr);
            membership.ipv6mr_interface = iface.index;
            if (setsockopt(result->fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, reinterpret_cast<const char*>(&membership), sizeof(membership))) {
                return fail();
            }
        }
    }
#ifdef _WIN32
    unsigned long nonblocking = 1;
    if (ioctlsocket(result->fd, FIONBIO, &nonblocking)) {
        return fail();
    }
#else
    const int flags = fcntl(result->fd, F_GETFL, 0);
    if (flags < 0 || fcntl(result->fd, F_SETFL, flags | O_NONBLOCK)) {
        return fail();
    }
#endif
    return result;
}

bool send_query(QuerySocket& socket, uint16_t type, std::string_view name, std::vector<std::string>& warnings)
{
    alignas(uint32_t) std::array<uint8_t, 1024> buffer { };
    mdns_header_t header { };
    header.questions = htons(1);
    std::memcpy(buffer.data(), &header, sizeof(header));
    mdns_string_table_t table { };
    // 使用库的 question builder；仅收发适配自行处理，避免库 int socket 缩窄 Windows SOCKET，
    // 也避免其 IPv6 socket helper 把组播接口索引固定成 0。
    auto* end = mdns_answer_add_question_unicast(
        buffer.data(),
        buffer.size(),
        buffer.data() + sizeof(header),
        static_cast<mdns_record_type_t>(type),
        name.data(),
        name.size(),
        &table);
    if (!end) {
        warn(warnings, "mDNS query name is too long");
        return false;
    }
    sockaddr_storage destination { };
    const int family = socket.iface.local.ss_family;
    if (family == AF_INET) {
        auto* address = reinterpret_cast<sockaddr_in*>(&destination);
        address->sin_family = AF_INET;
        address->sin_port = htons(MDNS_PORT);
        inet_pton(AF_INET, "224.0.0.251", &address->sin_addr);
    }
    else {
        auto* address = reinterpret_cast<sockaddr_in6*>(&destination);
        address->sin6_family = AF_INET6;
        address->sin6_port = htons(MDNS_PORT);
        address->sin6_scope_id = socket.iface.index;
        inet_pton(AF_INET6, "ff02::fb", &address->sin6_addr);
    }
    const auto length = static_cast<mdns_size_t>(static_cast<uint8_t*>(end) - buffer.data());
    if (sendto(
            socket.fd,
            reinterpret_cast<const char*>(buffer.data()),
            length,
            0,
            reinterpret_cast<const sockaddr*>(&destination),
            family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6))
        < 0) {
        warn(warnings, std::string("mDNS query on interface ") + socket.iface.name + ": " + transport::socket_error_message());
        return false;
    }
    socket.sent[{ type, std::string(name) }] = detail::RecordCache::Clock::now();
    return true;
}

} // namespace

BrowseResult browse(const BrowseOptions& options)
{
    BrowseResult result;
    const auto stop_requested = [&] {
        if (!result.cancelled && options.should_cancel) {
            result.cancelled = options.should_cancel();
        }
        return result.cancelled;
    };
    result.cancelled = stop_requested();
    if (result.cancelled || options.timeout.count() == 0) {
        return result;
    }
    if (options.timeout.count() < 0 || options.timeout > std::chrono::seconds(60)) {
        result.warnings.push_back("mDNS timeout must be between 0 and 60000 ms");
        return result;
    }
    try {
        std::string error;
        if (!transport::initialize_sockets(error)) {
            result.warnings.push_back(std::move(error));
            return result;
        }
        const auto deadline = detail::RecordCache::Clock::now() + options.timeout;
        std::vector<std::unique_ptr<QuerySocket>> sockets;
        for (const auto& iface : interfaces(result.warnings)) {
            if (stop_requested()) {
                break;
            }
            auto socket = open_socket(iface, result.warnings);
            if (socket && send_query(*socket, MDNS_RECORDTYPE_PTR, kService, result.warnings)) {
                sockets.push_back(std::move(socket));
            }
        }
        if (sockets.empty()) {
            result.cancelled = stop_requested();
            if (!result.cancelled) {
                warn(result.warnings, "No multicast interface is available for mDNS");
            }
            return result;
        }
        result.available = true;
        detail::RecordCache cache;
        alignas(uint32_t) std::array<uint8_t, kMaxPacket + 1> packet { };
        size_t received = 0;
        while (!stop_requested() && detail::RecordCache::Clock::now() < deadline) {
#ifdef _WIN32
            std::vector<WSAPOLLFD> ready;
            for (const auto& socket : sockets) {
                ready.push_back({ socket->fd, POLLRDNORM, 0 });
            }
#else
            std::vector<pollfd> ready;
            for (const auto& socket : sockets) {
                ready.push_back({ socket->fd, POLLIN, 0 });
            }
#endif
            const auto now = detail::RecordCache::Clock::now();
            const int wait_ms =
                static_cast<int>(std::min<int64_t>(50, std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count()));
#ifdef _WIN32
            const int available = WSAPoll(ready.data(), static_cast<ULONG>(ready.size()), std::max(0, wait_ms));
#else
            const int available = poll(ready.data(), static_cast<nfds_t>(ready.size()), std::max(0, wait_ms));
#endif
            if (available < 0) {
                if (transport::socket_error() == transport::kSocketInterrupted) {
                    continue;
                }
                warn(result.warnings, std::string("mDNS receive wait: ") + transport::socket_error_message());
                break;
            }
            for (size_t i = 0; i < sockets.size() && !stop_requested(); ++i) {
                if (!ready[i].revents) {
                    continue;
                }
                // 每轮每个 socket 至多读取 16 包；高流量接口不能拖延取消和其他接口。
                for (int batch = 0; batch < 16 && !stop_requested(); ++batch) {
                    sockaddr_storage from { };
                    socklen_t from_size = sizeof(from);
                    const auto size = recvfrom(
                        sockets[i]->fd,
                        reinterpret_cast<char*>(packet.data()),
                        static_cast<int>(packet.size()),
                        0,
                        reinterpret_cast<sockaddr*>(&from),
                        &from_size);
                    if (size < 0) {
                        if (!transport::socket_read_timed_out(transport::socket_error())) {
                            warn(
                                result.warnings,
                                std::string("mDNS receive on interface ") + sockets[i]->iface.name + ": "
                                    + transport::socket_error_message());
                        }
                        break;
                    }
                    if (++received > kMaxPackets) {
                        break;
                    }
                    if (size <= 0 || static_cast<size_t>(size) > kMaxPacket) {
                        continue;
                    }
                    // 只接受 mDNS responder 的源端口；广播内容仍是不受信任的发现信息。
                    const auto port = from.ss_family == AF_INET    ? reinterpret_cast<const sockaddr_in*>(&from)->sin_port
                                      : from.ss_family == AF_INET6 ? reinterpret_cast<const sockaddr_in6*>(&from)->sin6_port
                                                                   : uint16_t { 0 };
                    if (ntohs(port) != MDNS_PORT) {
                        continue;
                    }
                    cache.add_packet(
                        sockets[i]->iface.cache_index,
                        sockets[i]->iface.name,
                        { packet.data(), static_cast<size_t>(size) },
                        detail::RecordCache::Clock::now(),
                        sockets[i]->iface.ipv6_index);
                }
            }
            if (received > kMaxPackets) {
                warn(result.warnings, "mDNS packet limit reached");
                break;
            }
            const auto query_time = detail::RecordCache::Clock::now();
            for (const auto& socket : sockets) {
                auto queries = cache.queries(socket->iface.cache_index);
                queries.emplace_back(MDNS_RECORDTYPE_PTR, std::string(kService));
                for (const auto& [type, name] : queries) {
                    if (stop_requested() || detail::RecordCache::Clock::now() >= deadline) {
                        break;
                    }
                    const auto found = socket->sent.find({ type, name });
                    if (found == socket->sent.end() || query_time - found->second >= std::chrono::seconds(1)) {
                        send_query(*socket, type, name, result.warnings);
                    }
                }
            }
        }
        result.cancelled = stop_requested();
        result.advertisements = cache.snapshot();
        for (const auto& warning : cache.warnings()) {
            warn(result.warnings, warning);
        }
    }
    catch (const std::exception& error) {
        warn(result.warnings, std::string("mDNS browse: ") + error.what());
    }
    result.cancelled = stop_requested();
    return result;
}

namespace
{
constexpr std::string_view kHostService = "_remotepairing-pairable-host._tcp.local.";
constexpr uint32_t kHostTtl = 120;

bool host_label(std::string_view value)
{
    const auto alnum = [](unsigned char ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9');
    };
    if (value.empty() || value.size() > 63 || !alnum(value.front()) || !alnum(value.back())) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [&](unsigned char ch) { return alnum(ch) || ch == '-'; });
}

bool valid_host(const HostAdvertisement& host, std::string& error)
{
    if (!host_label(host.instance_identifier)) {
        error = "mDNS advertiser identifier must be a single ASCII DNS label";
        return false;
    }
    if (!host.port || (!host.advertise_ipv4 && !host.advertise_ipv6)) {
        error = "mDNS advertiser requires a listening TCP port and address family";
        return false;
    }
    const std::array<std::pair<std::string_view, std::string_view>, 7> fields { { { "name", host.name },
                                                                                  { "identifier", host.instance_identifier },
                                                                                  { "authTag", host.auth_tag },
                                                                                  { "model", host.model },
                                                                                  { "flags", "1" },
                                                                                  { "ver", "26" },
                                                                                  { "minVer", "17" } } };
    for (const auto& [key, value] : fields) {
        if (value.empty() || key.size() + 1 + value.size() > 255) {
            error = "mDNS advertiser has an empty or oversized TXT field";
            return false;
        }
    }
    return true;
}

bool dns_name(std::string_view name)
{
    if (name.empty() || name.size() > 253 || name.back() != '.' || name.find('\0') != std::string_view::npos) {
        return false;
    }
    size_t at = 0;
    while (at < name.size()) {
        const auto end = name.find('.', at);
        if (end == std::string_view::npos || end == at || end - at > 63) {
            return false;
        }
        at = end + 1;
    }
    return true;
}

struct HostQuestion
{
    std::string name;
    uint16_t type = 0;
    uint16_t cls = 0;
};

std::vector<uint8_t> encode_host(
    const detail::HostRecords& input,
    uint32_t ttl,
    const std::vector<HostQuestion>& questions,
    uint16_t id,
    bool legacy,
    std::string& error)
{
    error.clear();
    if (!valid_host(input.host, error)) {
        return { };
    }
    const auto identity = lower(input.host.instance_identifier);
    if (!dns_name(input.service_instance) || !dns_name(input.target)
        || lower(input.service_instance) != identity + "." + std::string(kHostService) || lower(input.target) != identity + ".local.") {
        error = "mDNS advertiser has an invalid service or target DNS name";
        return { };
    }
    if (input.addresses.size() > kMaxAddresses) {
        error = "mDNS advertiser address limit exceeded";
        return { };
    }
    auto text = [](std::string_view value) {
        return mdns_string_t { value.data(), value.size() };
    };
    std::vector<mdns_record_t> records;
    records.reserve(9 + input.addresses.size());
    mdns_record_t ptr { };
    ptr.name = text(kHostService);
    ptr.type = MDNS_RECORDTYPE_PTR;
    ptr.data.ptr.name = text(input.service_instance);
    ptr.rclass = MDNS_CLASS_IN;
    ptr.ttl = ttl;
    records.push_back(ptr);
    mdns_record_t srv { };
    srv.name = text(input.service_instance);
    srv.type = MDNS_RECORDTYPE_SRV;
    srv.data.srv.name = text(input.target);
    srv.data.srv.port = input.host.port;
    srv.rclass = MDNS_CLASS_IN | (legacy ? 0 : MDNS_CACHE_FLUSH);
    srv.ttl = ttl;
    records.push_back(srv);
    const std::array<std::pair<std::string_view, std::string_view>, 7> fields { { { "name", input.host.name },
                                                                                  { "identifier", input.host.instance_identifier },
                                                                                  { "authTag", input.host.auth_tag },
                                                                                  { "model", input.host.model },
                                                                                  { "flags", "1" },
                                                                                  { "ver", "26" },
                                                                                  { "minVer", "17" } } };
    for (const auto& [key, value] : fields) {
        mdns_record_t txt { };
        txt.name = text(input.service_instance);
        txt.type = MDNS_RECORDTYPE_TXT;
        txt.data.txt.key = text(key);
        txt.data.txt.value = text(value);
        txt.rclass = srv.rclass;
        txt.ttl = ttl;
        records.push_back(txt);
    }
    size_t address_count = 0;
    std::set<std::string> unique;
    for (const auto& ip : input.addresses) {
        if (!unique.insert(ip).second) {
            continue;
        }
        const bool ipv6 = ip.find(':') != std::string::npos;
        if ((ipv6 && !input.host.advertise_ipv6) || (!ipv6 && !input.host.advertise_ipv4)) {
            continue;
        }
        mdns_record_t record { };
        record.name = text(input.target);
        record.type = ipv6 ? MDNS_RECORDTYPE_AAAA : MDNS_RECORDTYPE_A;
        record.rclass = srv.rclass;
        record.ttl = ttl;
        bool valid = false;
        if (ipv6) {
            auto& address = record.data.aaaa.addr;
            address.sin6_family = AF_INET6;
            valid = inet_pton(AF_INET6, ip.c_str(), &address.sin6_addr) == 1 && !IN6_IS_ADDR_UNSPECIFIED(&address.sin6_addr)
                    && !IN6_IS_ADDR_LOOPBACK(&address.sin6_addr) && !IN6_IS_ADDR_MULTICAST(&address.sin6_addr);
        }
        else {
            auto& address = record.data.a.addr;
            address.sin_family = AF_INET;
            valid = inet_pton(AF_INET, ip.c_str(), &address.sin_addr) == 1;
            const uint32_t value = ntohl(address.sin_addr.s_addr);
            valid = valid && value && value != 0xffffffff && (value >> 24) != 127 && (value >> 28) < 14;
        }
        if (!valid) {
            error = "mDNS advertiser has an invalid numeric address";
            return { };
        }
        records.push_back(record);
        ++address_count;
    }
    if (!address_count) {
        error = "mDNS advertiser has no address for a listening family";
        return { };
    }
    alignas(uint32_t) std::array<uint8_t, kMaxPacket> buffer { };
    mdns_header_t header { };
    header.query_id = htons(id);
    header.flags = htons(0x8400);
    header.questions = htons(static_cast<uint16_t>(questions.size()));
    header.answer_rrs = htons(static_cast<uint16_t>(mdns_answer_get_record_count(records.data(), records.size())));
    std::memcpy(buffer.data(), &header, sizeof(header));
    mdns_string_table_t table { };
    void* end = buffer.data() + sizeof(header);
    for (const auto& question : questions) {
        end = mdns_string_make(buffer.data(), buffer.size(), end, question.name.data(), question.name.size(), &table);
        if (!end || static_cast<uint8_t*>(end) + 4 > buffer.data() + buffer.size()) {
            end = nullptr;
            break;
        }
        end = mdns_htons(end, question.type);
        end = mdns_htons(end, MDNS_CLASS_IN);
    }
    for (const auto& record : records) {
        if (!end) {
            break;
        }
        end = mdns_answer_add_record(buffer.data(), buffer.size(), end, record, &table);
    }
    if (end) {
        end = mdns_answer_add_txt_record(buffer.data(), buffer.size(), end, records.data(), records.size(), srv.rclass, ttl, &table);
    }
    if (!end) {
        error = "mDNS advertiser packet is too large";
        return { };
    }
    return { buffer.data(), static_cast<uint8_t*>(end) };
}

bool send_host(QuerySocket& socket, std::span<const uint8_t> wire, const sockaddr_storage* peer, socklen_t peer_size, std::string& error)
{
    sockaddr_storage destination { };
    const int family = socket.iface.local.ss_family;
    if (peer) {
        destination = *peer;
    }
    else if (family == AF_INET) {
        auto* address = reinterpret_cast<sockaddr_in*>(&destination);
        address->sin_family = AF_INET;
        address->sin_port = htons(MDNS_PORT);
        inet_pton(AF_INET, "224.0.0.251", &address->sin_addr);
    }
    else {
        auto* address = reinterpret_cast<sockaddr_in6*>(&destination);
        address->sin6_family = AF_INET6;
        address->sin6_port = htons(MDNS_PORT);
        address->sin6_scope_id = socket.iface.index;
        inet_pton(AF_INET6, "ff02::fb", &address->sin6_addr);
    }
    const auto size = sendto(
        socket.fd,
        reinterpret_cast<const char*>(wire.data()),
        static_cast<mdns_size_t>(wire.size()),
        0,
        reinterpret_cast<const sockaddr*>(&destination),
        peer                ? peer_size
        : family == AF_INET ? sizeof(sockaddr_in)
                            : sizeof(sockaddr_in6));
    if (size < 0 || static_cast<size_t>(size) != wire.size()) {
        error = std::string("mDNS advertiser send: ") + transport::socket_error_message();
        return false;
    }
    return true;
}

} // namespace

std::vector<uint8_t> detail::host_announcement(const HostRecords& records, uint32_t ttl, std::string& error)
{
    return encode_host(records, ttl, { }, 0, false, error);
}

std::vector<detail::HostReply>
    detail::host_query_reply(const HostRecords& records, std::span<const uint8_t> query, bool legacy, std::string& error)
{
    error.clear();
    if (query.size() < sizeof(mdns_header_t) || query.size() > kMaxPacket) {
        error = "mDNS advertiser query has an invalid size";
        return { };
    }
    const uint16_t flags = mdns_ntohs(query.data() + 2);
    if (flags & 0x8000) {
        return { }; // 不回应其他公告或自己的回应。
    }
    if (flags & 0x780f) {
        return { };
    }
    const auto count = mdns_ntohs(query.data() + 4);
    if (count > 32) {
        error = "mDNS advertiser question limit exceeded";
        return { };
    }
    size_t offset = sizeof(mdns_header_t);
    std::vector<HostQuestion> questions;
    bool unicast = false, multicast = false;
    for (unsigned i = 0; i < count; ++i) {
        const size_t before = offset;
        std::array<char, 256> name { };
        const auto value = mdns_string_extract(query.data(), query.size(), &offset, name.data(), name.size());
        if (before == offset || !value.length || value.length >= name.size() || offset + 4 > query.size()) {
            error = "mDNS advertiser query is truncated or has an invalid name";
            return { };
        }
        HostQuestion question { std::string(value.str, value.length),
                                mdns_ntohs(query.data() + offset),
                                mdns_ntohs(query.data() + offset + 2) };
        if (!dns_name(question.name)) {
            error = "mDNS advertiser query has an invalid DNS name";
            return { };
        }
        offset += 4;
        const auto cls = question.cls & 0x7fff;
        if (cls != MDNS_CLASS_IN && cls != MDNS_CLASS_ANY) {
            continue;
        }
        const auto normalized = lower(question.name);
        const bool any = question.type == MDNS_RECORDTYPE_ANY;
        const bool matches = (normalized == kHostService && (any || question.type == MDNS_RECORDTYPE_PTR))
                             || (normalized == lower(records.service_instance)
                                 && (any || question.type == MDNS_RECORDTYPE_SRV || question.type == MDNS_RECORDTYPE_TXT))
                             || (normalized == lower(records.target)
                                 && (any || (question.type == MDNS_RECORDTYPE_A && records.host.advertise_ipv4)
                                     || (question.type == MDNS_RECORDTYPE_AAAA && records.host.advertise_ipv6)));
        if (!matches) {
            continue;
        }
        questions.push_back(std::move(question));
        if (legacy || (questions.back().cls & MDNS_UNICAST_RESPONSE)) {
            unicast = true;
        }
        else {
            multicast = true;
        }
    }
    if (questions.empty()) {
        return { };
    }
    std::vector<HostReply> replies;
    if (legacy) {
        auto packet = encode_host(records, 10, questions, mdns_ntohs(query.data()), true, error);
        if (!packet.empty()) {
            replies.push_back({ std::move(packet), true });
        }
    }
    else {
        auto packet = encode_host(records, kHostTtl, { }, 0, false, error);
        if (packet.empty()) {
            return { };
        }
        if (unicast) {
            replies.push_back({ packet, true });
        }
        if (multicast) {
            replies.push_back({ std::move(packet), false });
        }
    }
    return replies;
}

struct Advertiser::Impl
{
    struct Binding
    {
        std::unique_ptr<QuerySocket> socket;
        detail::HostRecords records;
    };

    std::vector<Binding> bindings;
    detail::RecordCache::Clock::time_point next_announcement;

    bool announce(uint32_t ttl, std::string& error)
    {
        bool sent = false;
        std::string first_error;
        for (const auto& binding : bindings) {
            auto packet = detail::host_announcement(binding.records, ttl, error);
            if (!packet.empty() && send_host(*binding.socket, packet, nullptr, 0, error)) {
                sent = true;
            }
            else if (first_error.empty()) {
                first_error = error;
            }
        }
        error = sent ? std::string { } : first_error;
        return sent;
    }
};

Advertiser::Advertiser(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl))
{
}

Advertiser::~Advertiser()
{
    try {
        if (impl_) {
            std::string ignored;
            impl_->announce(0, ignored);
        }
    }
    catch (...) {
    }
}

std::unique_ptr<Advertiser> Advertiser::start(const HostAdvertisement& host, std::string& error)
{
    error.clear();
    if (!valid_host(host, error)) {
        return { };
    }
    try {
        if (!transport::initialize_sockets(error)) {
            return { };
        }
        auto impl = std::make_unique<Impl>();
        std::vector<std::string> warnings;
        const auto local_interfaces = interfaces(warnings);
        for (const auto& iface : local_interfaces) {
            if ((iface.local.ss_family == AF_INET && !host.advertise_ipv4) || (iface.local.ss_family == AF_INET6 && !host.advertise_ipv6)) {
                continue;
            }
            detail::HostRecords records { host,
                                          host.instance_identifier + "." + std::string(kHostService),
                                          host.instance_identifier + ".local.",
                                          { } };
            for (const auto& address : local_interfaces) {
                if (address.cache_index != iface.cache_index) {
                    continue;
                }
                if ((address.local.ss_family == AF_INET && !host.advertise_ipv4)
                    || (address.local.ss_family == AF_INET6 && !host.advertise_ipv6)) {
                    continue;
                }
                std::array<char, INET6_ADDRSTRLEN> text { };
                const void* raw = address.local.ss_family == AF_INET
                                      ? static_cast<const void*>(&reinterpret_cast<const sockaddr_in*>(&address.local)->sin_addr)
                                      : static_cast<const void*>(&reinterpret_cast<const sockaddr_in6*>(&address.local)->sin6_addr);
                if (inet_ntop(address.local.ss_family, raw, text.data(), text.size())) {
                    records.addresses.emplace_back(text.data());
                }
            }
            if (detail::host_announcement(records, kHostTtl, error).empty()) {
                continue;
            }
            auto socket = open_socket(iface, warnings, true);
            if (socket) {
                impl->bindings.push_back({ std::move(socket), std::move(records) });
            }
        }
        if (impl->bindings.empty()) {
            error = warnings.empty() ? std::string("No multicast interface is available for mDNS") : warnings.front();
            return { };
        }
        auto advertiser = std::unique_ptr<Advertiser>(new Advertiser(std::move(impl)));
        if (!advertiser->impl_->announce(kHostTtl, error)) {
            return { };
        }
        advertiser->impl_->next_announcement = detail::RecordCache::Clock::now() + std::chrono::seconds(1);
        return advertiser;
    }
    catch (const std::exception& exception) {
        error = std::string("mDNS advertiser: ") + std::string(exception.what());
        return { };
    }
}

bool Advertiser::poll(int timeout_ms, std::string& error)
{
    error.clear();
    if (timeout_ms < 0 || timeout_ms > 60000) {
        error = "mDNS timeout must be between 0 and 60000 ms";
        return false;
    }
    const auto deadline = detail::RecordCache::Clock::now() + std::chrono::milliseconds(timeout_ms);
    const auto now = detail::RecordCache::Clock::now();
    if (now >= impl_->next_announcement) {
        if (!impl_->announce(kHostTtl, error)) {
            return false;
        }
        impl_->next_announcement = now + std::chrono::seconds(60);
    }
#ifdef _WIN32
    std::vector<WSAPOLLFD> ready;
    for (const auto& binding : impl_->bindings) {
        ready.push_back({ binding.socket->fd, POLLRDNORM, 0 });
    }
#else
    std::vector<pollfd> ready;
    for (const auto& binding : impl_->bindings) {
        ready.push_back({ binding.socket->fd, POLLIN, 0 });
    }
#endif
    const auto until_refresh = std::chrono::duration_cast<std::chrono::milliseconds>(impl_->next_announcement - now).count();
    const int wait_ms = static_cast<int>(std::max<int64_t>(0, std::min<int64_t>({ 50, timeout_ms, until_refresh })));
#ifdef _WIN32
    const int count = WSAPoll(ready.data(), static_cast<ULONG>(ready.size()), wait_ms);
#else
    const int count = ::poll(ready.data(), static_cast<nfds_t>(ready.size()), wait_ms);
#endif
    if (count < 0) {
        if (transport::socket_error() == transport::kSocketInterrupted) {
            return true;
        }
        error = std::string("mDNS advertiser receive wait: ") + transport::socket_error_message();
        return false;
    }
    alignas(uint32_t) std::array<uint8_t, kMaxPacket + 1> packet { };
    for (size_t i = 0; i < ready.size(); ++i) {
        if (!ready[i].revents) {
            continue;
        }
        auto& binding = impl_->bindings[i];
        for (int batch = 0; batch < 16; ++batch) {
            sockaddr_storage peer { };
            socklen_t peer_size = sizeof(peer);
            const auto size = recvfrom(
                binding.socket->fd,
                reinterpret_cast<char*>(packet.data()),
                static_cast<int>(packet.size()),
                0,
                reinterpret_cast<sockaddr*>(&peer),
                &peer_size);
            if (size < 0) {
                if (!transport::socket_read_timed_out(transport::socket_error())) {
                    error = std::string("mDNS advertiser receive: ") + transport::socket_error_message();
                    return false;
                }
                break;
            }
            if (size > 0 && static_cast<size_t>(size) <= kMaxPacket) {
                const uint16_t port = peer.ss_family == AF_INET ? reinterpret_cast<const sockaddr_in*>(&peer)->sin_port
                                                                : reinterpret_cast<const sockaddr_in6*>(&peer)->sin6_port;
                std::string parse_error;
                const auto replies = detail::host_query_reply(
                    binding.records,
                    { packet.data(), static_cast<size_t>(size) },
                    ntohs(port) != MDNS_PORT,
                    parse_error);
                // 不受信任的坏查询只丢弃；不能用一个坏包中断本机配对监听。
                for (const auto& reply : replies) {
                    if (!send_host(*binding.socket, reply.packet, reply.unicast ? &peer : nullptr, peer_size, error)) {
                        return false;
                    }
                }
            }
            if (detail::RecordCache::Clock::now() >= deadline) {
                break;
            }
        }
        if (detail::RecordCache::Clock::now() >= deadline) {
            break;
        }
    }
    return true;
}

} // namespace maa::ios::wifi::mdns
