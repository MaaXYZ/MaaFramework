#pragma once

#include <optional>
#include <string>

#include "remote/Rsd.h"
#include "wifi/Rppairing.h"
#include "xpc/XpcValue.h"
#include "json/Json.h"

namespace maa::ios::remote
{

/// RemotePairing 信封在 RemoteXPC 上的外层类型名。设备按它认这条通道上跑的是什么。
inline constexpr std::string_view kPairingEnvelopeType = "RemotePairing.ControlChannelMessageEnvelope";

/// JSON → XPC，带 RemotePairing 的三条类型规则。
///
/// 同一套信封在两种载体上长得不一样（docs §25.8）：字节流上是 JSON 文本，TLV 与密文
/// 这些二进制字段用 base64 字符串表示；RemoteXPC 上是一个 XPC 字典，同样的字段必须是
/// **真的 XPC data**，序号必须是 XPC uint64。类型不对设备当场把连接 invalidate 掉，
/// 现场看起来跟"它不喜欢我们的字段"一模一样，所以这三条规则值得单独立个函数。
///
///   sequenceNumber              整数    → uint64（设备按无符号读）
///   message.streamEncrypted._0  base64  → data
///   …pairingData._0.data        base64  → data
///
/// 其余一一对应：字符串→string、整数→int64、真值→bool、数组/对象→array/dict。
std::optional<xpc::Value> json_to_xpc(const json::Value& value, std::string& err);

/// XPC → JSON。这个方向没有歧义：data 一律还原成 base64 字符串，uint64/int64 一律
/// 变成 JSON 整数，UUID 变成 8-4-4-4-12 文本——正好是 Rppairing 那套解析代码在字节流
/// 载体上看到的形状，所以配对逻辑本身不用知道自己在哪种载体上。
std::optional<json::Value> xpc_to_json(const xpc::Value& value, std::string& err);

/// RemoteXPC 载体上的配对通道：把信封裹进 `{mangledTypeName, value}` 发出去，收回来
/// 再拆出 `value`。
///
/// 这条载体上发送**不带** WANTING_REPLY（参考实现在这条通道上就是这么发的，设备的
/// 回信照样来），而且发与收必须能分开：设备对一次发送可能回两条（先
/// `awaitingUserConsent` 再给数据），也可能一条都不回（iOS 27 的 verify 探针）。
/// 多收下来的那条消息存在 `pending_` 里，下一次 `read_envelope` 直接取走，不会因为
/// "探一眼有没有回信"就把回信吃掉。
class XpcPairingCarrier final : public wifi::EnvelopeCarrier
{
public:
    XpcPairingCarrier(ServiceConnection& conn, int timeout_ms)
        : conn_(conn)
        , timeout_ms_(timeout_ms)
    {
    }

    bool write_envelope(const json::Value& envelope, std::string& err) override;
    std::optional<json::Value> read_envelope(std::string& err) override;
    bool wait_readable(int ms, std::string& err) override;

private:
    bool wait_one(int ms, xpc::Value& out, std::string& err);

    ServiceConnection& conn_;
    int timeout_ms_;
    std::optional<xpc::Value> pending_;
};

} // namespace maa::ios::remote
