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
    /// 本条流声明的客户端 SSRC：视频放在 VideoSettings.f1，音频放在媒体参数的 f3.f1。
    /// 发送 RTCP 时，以 answer 的 RemoteSSRC 作为客户端发送者 SSRC。
    /// 视频和音频分别使用自己的 SSRC，不与 ClientSessionID UUID 混用。
    uint32_t session_id = 0;
    /// avcMediaStreamOptionCallID，本次协商调用的文本追踪号，与 SSRC 和 ClientSessionID 分开。
    std::string call_id;

    /// 选择音频或视频 offer。音频默认使用 negotiator mode 10、设置消息 f3；
    /// 视频使用 mode 5、设置消息 f5（VideoSettings）。音频设置包含 f1=SSRC、f4=24191，
    /// f4 的具体含义尚未确认。两种 offer 共享媒体参数容器中的默认字段和码率表，
    /// 编码器能力条目位于视频设置中。StreamSession::start() 为每次协商生成 CallID。
    /// 共享 ClientSessionID 不是当前实现维持视频会话的前提；每条流仍需发送自己的 RTCP。
    bool is_audio = false;
    /// 音频路由：false 使用 NearbySystemAudio（mode 10），将播放转到电脑；
    /// true 使用旧 CoreDeviceSystemAudio（mode 6），保留手机播放。
    /// 只改变音频 offer 的外层 mode，媒体参数、主机身份及视频 offer 均不受影响。
    /// 切路由可能暂停播放器。raw_offer 重放不受该选项影响。
    bool audio_dup = false;

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

    /// 码率阶梯变体，供协议探针使用；正常路径保持 0。
    /// 0 = 保留原表；1 = 删除 f2=6000000 的记录；2 = 将该值改为 60000000；
    /// 3 = 删除 1000 <= f2 < 20000000 的记录，仍保留 f2=0、299 等其他参数记录。
    /// 此表不是通用码率或分辨率设置接口，也不能保证单个 NAL 的长度上限。
    /// VideoToolbox 适配使用两字节长度前缀，超过 65535 字节的 NAL 需要软件解码。
    int rate_variant = 0;
};

/// 将以上参数序列化为 XML plist，供 startmediastream 的 negotiatorOffer 使用 XPC Data 携带。
/// 两个 Data 字段在 XML 中使用 Base64；压缩媒体参数和 endpoint protobuf 的内容不变。
[[nodiscard]] std::vector<uint8_t> build_negotiator_offer(const Offer& offer);

} // namespace maa::ios::media
