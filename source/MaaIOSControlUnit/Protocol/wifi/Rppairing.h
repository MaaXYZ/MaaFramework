#pragma once

#include "i18n/Translation.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "transport/Usbmux.h"
#include "wifi/Crypto.h"
#include "json/Json.h"

namespace maa::ios::wifi
{

/// 字节流。RPPairing 通道不关心底下是普通 TCP 还是 lockdown 那条 TLS 服务连接，
/// 所以这里只提"写全 / 读够"两个动作。
class ByteStream
{
public:
    virtual ~ByteStream() = default;
    virtual bool write_all(const void* data, size_t len, std::string& err) = 0;
    virtual bool read_exact(void* data, size_t len, std::string& err) = 0;

    /// 在 ms 毫秒内会不会有字节可读。pair-setup 的 verify 探针要用它判"设备这次
    /// 到底回不回话"（iOS 27 上签名有效但 identifier 未知时它**不回**，docs §25.6）。
    virtual bool wait_readable(int ms, std::string& err)
    {
        (void)ms;
        err = MAA_IOS_TR("This stream does not support waiting for readability");
        return false;
    }
};

/// 包住一个已连上的 socket。
class SocketStream final : public ByteStream
{
public:
    explicit SocketStream(transport::Socket& sock)
        : sock_(sock)
    {
    }

    bool write_all(const void* data, size_t len, std::string& err) override { return sock_.write_all(data, len, err); }

    bool read_exact(void* data, size_t len, std::string& err) override { return sock_.read_exact(data, len, err); }

    bool wait_readable(int ms, std::string& err) override { return sock_.wait_readable(ms, err); }

private:
    transport::Socket& sock_;
};

/// 配对消息构造辅助函数；值存储使用 nlohmann/json。
json::Value j_str(std::string_view s);
json::Value j_int(int64_t v);
json::Value j_bool(bool v);
json::Value j_arr(std::vector<json::Value> items);
json::Value j_obj(std::vector<std::pair<std::string, json::Value>> kv);

/// 主机报的"我自己会说这套协议的版本"。设备 TXT 里那个 `ver=26` 是**它**的版本，
/// 不是我们该发的值——实测对 iOS 27 发 19 能用（docs §22.3）。pair-setup 与
/// pair-verify 的 handshake 用的是同一条，所以放在这里共用。
inline constexpr int64_t kWireProtocolVersion = 19;

/// 信封的搬运工。同一套 RemotePairing 信封有两种载体：
///   字节流上的 `"RPPairing" + u16 长度 + JSON 文本`（Wi-Fi 手动配对面 49152、
///   lockdown 服务 `com.apple.dt.remotepairingdeviced.lockdown`）；
///   RemoteXPC 上的一个 XPC 字典（隧道内的 untrusted.tunnelservice）。
/// iOS 27 只在后者上收 pair-setup，前者一律 M1 即 invalidated（docs §25.6/§25.8），
/// 所以配对的逻辑必须与载体解耦——这层就是那个缝。
class EnvelopeCarrier
{
public:
    virtual ~EnvelopeCarrier() = default;
    /// 发一条完整信封（`{message, originatedBy, sequenceNumber}`）。
    virtual bool write_envelope(const json::Value& envelope, std::string& err) = 0;
    /// 收一条完整信封。
    virtual std::optional<json::Value> read_envelope(std::string& err) = 0;
    /// ms 毫秒内有没有回信可读。pair-setup 的 verify 探针要用它判"设备这次到底
    /// 回不回话"（iOS 27 上签名有效但 identifier 未知时它**不回**，docs §25.6）。
    virtual bool wait_readable(int ms, std::string& err) = 0;
};

inline constexpr std::string_view kRpPairingMagic = "RPPairing";

/// 字节流载体：帧格式实测来自 iOS 27 上的往返，不是文档。
class FramedCarrier final : public EnvelopeCarrier
{
public:
    explicit FramedCarrier(ByteStream& io)
        : io_(io)
    {
    }

    bool write_envelope(const json::Value& envelope, std::string& err) override;
    std::optional<json::Value> read_envelope(std::string& err) override;

    bool wait_readable(int ms, std::string& err) override { return io_.wait_readable(ms, err); }

private:
    ByteStream& io_;
};

/// RemotePairing 控制通道：信封、序号、主密钥。
///
/// 信封有两种：`message.plain._0`（明文，里面再套 request/event/response）与
/// `message.streamEncrypted._0`（ChaCha20-Poly1305 的一段密文；字节流载体上是它的
/// base64 文本，XPC 载体上是 XPC data）。
///
/// 序号那一条规矩最容易写错：**只有明文发送会推进 `sequenceNumber`**，配对完成后的
/// 加密请求全部沿用当时那个值不变。看着别扭，但对端按这个来——自己"顺手也加一"会
/// 在 createListener 那一步被拒，而且症状像"权限不够"。
class Rppairing
{
public:
    explicit Rppairing(EnvelopeCarrier& carrier)
        : carrier_(carrier)
    {
    }

    /// 装 pair-verify 得到的两条主密钥（各 32 字节），之后才能走加密往返。
    void install_main_keys(Bytes client_key, Bytes server_key);

    /// 发一条明文内部消息（`{"request":…}` 或 `{"event":…}`），不收回复。
    bool send_plain(const json::Value& inner, std::string& err);

    /// 收一条信封。明文返回其内部消息；加密的用**上一次请求的 nonce** 解开后返回。
    ///
    /// 这里刻意不做"超时也算一种结果"：控制通道上的回复要么来要么连接死了，
    /// 中间态没有意义，交给调用方的 socket 超时处理。
    std::optional<json::Value> receive(std::string& err);

    /// 一次明文往返。
    std::optional<json::Value> plain_roundtrip(const json::Value& inner, std::string& err);

    /// 一次加密往返：`request` 形如 `{"request":{…}}`，返回解出来的 `response._1`。
    std::optional<json::Value> encrypted_roundtrip(const json::Value& request, std::string& err);

    /// ms 毫秒内有没有回信可读。pair-setup 的 verify 探针靠它区分"设备不回话"与
    /// "设备回了 Msg04"（iOS 27 上前者才是"没配对"的正常表现，docs §25.6）。
    bool reply_pending(int ms, std::string& err) { return carrier_.wait_readable(ms, err); }

    [[nodiscard]] uint64_t sequence() const { return sequence_; }

    [[nodiscard]] uint64_t encrypted_sequence() const { return encrypted_sequence_; }

private:
    bool send_envelope(const json::Value& message, std::string& err);

    EnvelopeCarrier& carrier_;
    uint64_t sequence_ = 0;
    uint64_t encrypted_sequence_ = 0;
    Bytes client_main_;
    Bytes server_main_;
    /// 上一次加密请求用的 nonce：设备的回信用同一个值，不是各自计数。
    std::string last_nonce_;
};

/// 把中间状态说给人听的回调（例如"设备正在等你在屏幕上点信任"）。空函数=不报。
using ProgressFn = std::function<void(std::string_view)>;

/// 只发不收：pair-setup 的 verify 探针发完 PV-Msg03 不能傻等回信（设备在"签名有效但
/// identifier 未知"时不回），发与收必须能拆开。
bool send_pairing_data(
    Rppairing& channel,
    const Bytes& tlv,
    std::string_view kind,
    bool start_new_session,
    std::string& err,
    std::string_view sending_host = "");

/// 发一条 pairingData 事件，回一条 pairingData 事件，返回解出来的 TLV 字节。
/// pair-verify 与 pair-setup 共用这条管道；`sending_host` 非空时带上 sendingHost 键
/// （pair-setup 的三条都要它，verify 不带）。
///
/// 设备的回信有三种形状，都得当正常情况处理：
///   pairingData            —— 直接就是下一步的 TLV；
///   awaitingUserConsent    —— 屏幕上弹了"信任这台电脑？"，要**再收一帧**才是 TLV；
///   pairingRejectedWithError —— 那句 `NSLocalizedDescription` 是人话，直接抬出去，
///                               别让调用方去猜"为什么没有 pairingData"。
std::optional<Bytes> pairing_data_roundtrip(
    Rppairing& channel,
    const Bytes& tlv,
    std::string_view kind,
    bool start_new_session,
    std::string& err,
    std::string_view sending_host = "",
    const ProgressFn& progress = nullptr);

} // namespace maa::ios::wifi
