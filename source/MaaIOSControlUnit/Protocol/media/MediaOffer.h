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
/// 字段与码流记录见 [CoreDevice §11](../../docs/coredevice.md#11-屏幕视频流的线上细节实测iphone-144--ios-270--usb)，
/// XML 的验证范围见 [BPLIST_COMPATIBILITY](../../docs/BPLIST_COMPATIBILITY.md#当前结论)。
struct Offer
{
    /// 本条流声明的客户端 SSRC：视频放在 VideoSettings.f1，音频放在媒体参数的 f3.f1。
    /// 字段名中的 session 为历史命名。已测设备将该值原样回显为 answer 的 RemoteSSRC；
    /// 发送 RTCP 时，以本次 answer 的 RemoteSSRC 作为客户端发送者 SSRC。
    /// 视频和音频分别使用自己的 SSRC，不与 ClientSessionID UUID 混用。
    uint32_t session_id = 0;
    /// avcMediaStreamOptionCallID，本次协商调用的文本追踪号，与 SSRC 和 ClientSessionID 分开。
    std::string call_id;

    /// 选择音频或视频 offer。音频默认使用 negotiator mode 10、设置消息 f3；
    /// 视频使用 mode 5、设置消息 f5（VideoSettings）。音频设置包含 f1=SSRC、f4=24191，
    /// f4 的具体含义尚未确认。两种 offer 共享媒体参数容器中的默认字段和码率表，
    /// 编码器能力条目位于视频设置中。StreamSession::start() 为每次协商生成 CallID。
    /// 编码形状与音视频并行验证见 [CoreDevice §17、§17.2](../../docs/coredevice.md#17-音频腿的编码鉴定实测iphone144--ios-270--usb)。
    /// 共享 ClientSessionID 不是当前实现维持视频会话的前提；每条流仍需发送自己的 RTCP。
    bool is_audio = false;
    /// 音频路由：false 使用 NearbySystemAudio（mode 10），将播放转到电脑；
    /// true 使用旧 CoreDeviceSystemAudio（mode 6），保留手机播放。
    /// 只改变音频 offer 的外层 mode，媒体参数、主机身份及视频 offer 均不受影响。
    /// mode 10 已在 iPhone14,4 / iOS 27 验证；切路由会暂时暂停播放器，需手动继续。
    /// 不据此假定旧系统或 Windows 已验证。raw_offer 重放不受该选项影响。
    bool audio_dup = false;

    /// 申报的主机型号、系统版本和构建号。默认值来自已有成功会话使用的身份组合，
    /// 不表示当前进程会读取本机信息或确认这些值与实际主机一致。
    /// 参考实现曾观察到更换型号后编码参数和停顿频率变化，因此更改这些值需要重新验证；
    /// 不能据此推断所有设备都使用相同的参数选择规则。
    std::string host_model = "Mac15,9";
    std::string host_os_version = "2205.3.1";
    std::string host_build = "25F80";

    /// 申报的 AVC / HEVC 能力串。当前默认不包含 VRAE:0。
    /// 参考实现的设备对照中，该 token 与编码器丢输入帧、帧率下降和拖影相关；
    /// 去掉后丢帧和帧率改善。这是已测配置的表现，不是协议禁止该 token 的证据。
    /// 观测条件见 [CoreDevice §6](../../docs/coredevice.md#6-实测踩到的坑)。
    std::string avc_features = "FLS;SW:1;";
    std::string hevc_features = "FLS;SW:1;";

    /// VideoSettings.f2（allowRTCPFB），当前默认 false，字段的完整作用尚未确认。
    /// 修正 UDP 发送后，已测设备在默认值 false 下可通过 RR 续期并响应 PLI。
    /// 对照中切换此位未改变 RR 续期；不能将它解释为所有 RTCP 的总开关。
    /// 旧 FIR 失败使用了非标准包，不能据此判断此位控制标准 FIR。格式修正后已观察到响应。
    /// 当前产品使用 RR 和 PLI；标准 FIR 尚未纳入恢复策略。
    /// 实验范围见 [CoreDevice §13 的 UDP 修复后对照](../../docs/coredevice.md#udp-修复后的反馈对照)。
    bool allow_rtcp_fb = false;
    /// VideoSettings.f7，即 ltrpEnabled（长期参考图）。当前默认关闭。
    /// 已测设备会在 answer 的 IsltrpEnabled 中回显此位；参考实现的对照中，
    /// 开关此位未改变编码器丢帧数。该观测不能用于推断 RTCP 租期或其他设备的恢复行为。
    /// 相关协商对照见 [CoreDevice §13](../../docs/coredevice.md#13-停流关键帧请求与恢复实测)。
    bool ltrp_enabled = false;

    /// 码率阶梯变体，供 tools/bitrate_probe 对照；产品路径保持 0。
    /// 0 = 保留原表；1 = 删除 f2=6000000 的记录；2 = 将该值改为 60000000；
    /// 3 = 删除 1000 <= f2 < 20000000 的记录，仍保留 f2=0、299 等其他参数记录。
    /// 已测设备中，提高该档未提高 answer 的 TXMaxBitrate；删除后码率或帧率下降。
    /// 同轮分辨率参数对照也未改变编码尺寸，因此目前保留原表，不将它作为通用码率旋钮。
    /// 码率、分辨率和主机能力对照见 [CoreDevice §11](../../docs/coredevice.md#11-屏幕视频流的线上细节实测iphone-144--ios-270--usb)。
    /// 已测码流可能包含超过 65535 字节的单个 NAL。当前 VideoToolbox 适配使用两字节
    /// 长度前缀，遇到这种 NAL 需要软件解码后端；其他 VideoToolbox 配置需另行验证。
    int rate_variant = 0;
};

/// 将以上参数序列化为 XML plist，供 startmediastream 的 negotiatorOffer 使用 XPC Data 携带。
/// 两个 Data 字段在 XML 中使用 Base64；压缩媒体参数和 endpoint protobuf 的内容不变。
/// 视频、音频与生产路径对照见 [BPLIST_COMPATIBILITY 的真机对照](../../docs/BPLIST_COMPATIBILITY.md#真机对照)。
/// 该结果限定于文档所列设备、系统与请求，不表示其他 CoreDevice feature 都接受 XML。
[[nodiscard]] std::vector<uint8_t> build_negotiator_offer(const Offer& offer);

} // namespace maa::ios::media
