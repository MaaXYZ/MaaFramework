#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace maa::ios::media
{

/// CoreDevice 媒体协商 offer 的本地参数。
///
/// 外层使用 XML plist。MediaBlob 的 Data 包含 zlib 压缩的媒体参数 protobuf；
/// RemoteEndpointInfo 的 Data 直接包含主机信息 protobuf，不经 zlib 压缩。
/// protobuf 字段号和取值来自可用会话的观测；未确认含义的参数保留观测值。
/// 修改参数时需分别验证协商回复、实际码流和恢复行为，不能仅以 RPC 成功判断兼容性。
struct Offer
{
    /// 本条流声明的客户端 SSRC，放在 VideoSettings.f1。
    /// 发送 RTCP 时，以 answer 的 RemoteSSRC 作为客户端发送者 SSRC。
    uint32_t session_id = 0;
    /// avcMediaStreamOptionCallID，本次协商调用的文本追踪号，与 SSRC 和 ClientSessionID 分开。
    std::string call_id;

    /// 申报的主机型号、系统版本和构建号，不读取本机信息。
    /// 这些参数可能影响设备的编码选择。
    std::string host_model = "Mac15,9";
    std::string host_os_version = "2205.3.1";
    std::string host_build = "25F80";

    /// 申报的 AVC / HEVC 能力串。省略可能造成设备编码丢帧的 VRAE:0。
    std::string avc_features = "FLS;SW:1;";
    std::string hevc_features = "FLS;SW:1;";

    /// VideoSettings.f2（allowRTCPFB），默认 false；不作为 RR 续期或 PLI 的开关。
    /// 该字段的完整作用尚未确认。
    bool allow_rtcp_fb = false;
    /// VideoSettings.f7，即 ltrpEnabled（长期参考图）。当前默认关闭。
    /// 设备可在 answer 的 IsltrpEnabled 中回显此位。
    bool ltrp_enabled = false;
};

/// 将以上参数序列化为 XML plist，供 startmediastream 的 negotiatorOffer 使用 XPC Data 携带。
/// 两个 Data 字段在 XML 中使用 Base64；压缩媒体参数和 endpoint protobuf 的内容不变。
[[nodiscard]] std::vector<uint8_t> build_negotiator_offer(const Offer& offer);

} // namespace maa::ios::media
