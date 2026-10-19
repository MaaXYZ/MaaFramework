#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "media/MediaOffer.h"
#include "net/UdpSocket.h"
#include "remote/Device.h"

namespace maa::ios::media
{

/// 建立一条设备视频或音频媒体会话，并拥有该会话的 UDP 接收套接字。
///
/// UDP 端口先绑定，再调用 startmediastream，避免丢失设备立即发送的参数集和首个关键帧。
class StreamSession
{
public:
    struct Request
    {
        uint16_t receiver_port = 0; ///< 0 表示从 49152..65151 中随机选择端口，随后尝试绑定。
        /// 视频目标的显示器 ID；音频请求不携带显示器 options。
        uint32_t display_id = 1;
        /// 发往设备的 timeout，单位为秒，是 RTCP 空闲超时，不是等待起流 RPC 回复的时限。
        /// 对应 answer 的 RTCPTimeoutInterval；有效 RR 可重置计时器。
        ///
        /// 此低层接口默认使用 3600。产品 FramePump 和 AudioPump
        /// 显式使用 20 秒，并每秒发送 RR。默认值、保留起流连接或查询状态都不能替代 RTCP。
        /// 视频和音频各自有独立的会话与续期状态。
        ///
        /// nullopt 表示省略整个键，供协议探针使用；正常起流应提供此参数。
        std::optional<uint32_t> timeout_seconds = 3600;
        /// 可选的 sessionEventChannel，值为 16 字节 XPC UUID；未提供或为空时省略。
        /// 其事件通道的注册与对端语义尚未确认，不能把一个 UUID 本身当作保活机制。
        std::optional<std::vector<uint8_t>> session_event_channel;
        /// 申报的客户端能力位掩码，默认 140 来自可用请求的观测。
        /// 设备回复的 supportedFeatures=972 是设备能力，不能直接当作客户端应申报的值。
        /// 各位的含义尚未完整确认。
        uint64_t client_supported_features = 140;
        /// 选择音频请求：type 为 audio，省略显示器选择 options，本地 offer 默认 mode 10；
        /// offer.audio_dup 使用旧 mode 6，保留手机播放。
        /// 视频请求的 type 为 video，本地生成的 offer 使用 mode 5；raw_offer 自身的 mode 不改写。
        bool audio = false;
        /// avcMediaStreamOptionClientSessionID 的 16 字节 XPC UUID 数据；为空时为本次请求生成。
        /// 音视频分别使用独立 UUID，以便查询各自状态。
        /// 若共享 UUID，probe() 的 Alive 只能表示至少一条同 UUID 会话存在，无法区分音视频。
        std::vector<uint8_t> client_session_uuid;
        /// 普通路径的 offer 参数。start() 会根据 audio 设置 is_audio，并为本次协商
        /// 生成 session_id 和 call_id，覆盖传入 Offer 中这三个字段。
        Offer offer;
        /// 非空时跳过本地 XML offer 构造，原样作为 negotiatorOffer 的 XPC Data。
        /// 供抓包重放和 binary / XML 格式对照，不修改样本中的 SSRC、CallID 或其他内容。
        /// 此字段只提供 offer 内容，type、options、接收端口等仍由 Request 的其他字段构造。
        /// 调用方负责选择与 audio 请求类型相符的样本。
        /// 发送 RTCP 应使用本次 answer 的 SSRC，不能使用普通构造路径另外生成的身份。
        std::vector<uint8_t> raw_offer;
    };

    struct Started
    {
        /// answer 的 connection.sender.port，设备发送媒体使用的端口。
        /// 显式提供时须为 1..65535 的整数或十进制字符串；非法值使起流失败。
        /// 缺失时保留 0，仍可收包，但 UdpSocket 拒绝向 0 发送反馈。
        /// 当前 FramePump / AudioPump 不会用收包源端口自动补全反馈目的端口。
        uint16_t sender_port = 0;
        /// 协商的媒体 RTP payload type，来自 streamConfig.RxPayloadType 的低七位。
        /// 缺失时保留默认 100；音频也使用协商值。拆包前需区分同端口收到的裸 RTCP。
        uint8_t payload_type = 100;
        /// answer 的 streamConfig.LocalSSRC，名字采用设备视角。
        /// 已测值与设备发送的 RTP SSRC 一致；客户端 RTCP 报告块用它指明被报告的媒体源。
        uint32_t local_ssrc = 0;
        /// 是否实际协商了 LocalSSRC。显式 0 是合法源；缺失字段仍保留标量默认值，
        /// 收包方可在缺失时绑定第一条合法媒体 RTP，不把默认 0 误当成协商值。
        bool has_local_ssrc = false;
        /// answer 的 streamConfig.RemoteSSRC，表示客户端侧 SSRC。
        /// 已测设备回显 offer 声明的 SSRC，不应理解为设备总会另行分配新值。
        /// 客户端发送 RTCP 时使用本次回复的此值，避免与报告的会话身份不匹配。
        uint32_t remote_ssrc = 0;
        /// 同样以字段存在性区分缺失与合法 0，不能用 remote_ssrc 的数值判断缺失。
        bool has_remote_ssrc = false;
        /// 起流回复原文，供上层读取和记录完整协商结果。
        maa::ios::xpc::Value answer;
        /// 本次请求中 ClientSessionID 的 16 字节 XPC UUID 原文，用于 probe() 匹配会话。
        /// 当前 stop() 使用 stopAll，不按此 UUID 筛选。
        std::vector<uint8_t> session_uuid;
    };

    /// 区分请求失败与设备已接受请求但答复无法使用，供调用方决定是否重试。
    enum class StartStatus
    {
        /// 尚未得到 feature 成功确认。传输超时不能证明设备没有执行请求。
        NotConfirmed,
        /// feature 已成功，但答复解析失败；设备流可能已开始，需要停止续期并等租期释放。
        AcceptedInvalidAnswer,
        /// 请求成功，答复已解析为可用的 StreamSession 参数。
        Started,
    };

    /// 在已建立隧道并取得 RSD 目录的 Device 上起流，先绑定接收 UDP 端口。
    /// 失败时返回 nullptr，并通过 err 提供传输、CoreDevice 或回复解析错误。
    /// 设备已接受请求后若回复解析失败，只释放本地 UDP 套接字，不发送 stopAll，
    /// 避免停止并存的音视频会话。本次设备流可能已开始，依赖请求中的 RTCP 租期释放；
    /// 产品请求为 20 秒，低层 Request 默认 3600 秒，此处不更改租期。
    /// start_status 非空时写入明确结果，答复解析失败为 AcceptedInvalidAnswer；
    /// 调用方无需匹配 err 文本，也不应把这种结果当作普通连接失败反复重试。
    ///
    /// 默认通过 Device::feature 新建服务连接，调用结束后释放。on_conn 非空时借用
    /// 调用方持有的连接，不取得所有权，也不在调用结束后关闭它。
    /// 后续 status、stop 等请求使用新连接，避免设备结束原媒体会话；
    /// 保留连接本身不能替代周期 RTCP。
    static std::unique_ptr<StreamSession> start(
        maa::ios::remote::Device& device,
        const Request& request,
        std::string& err,
        bool verbose = false,
        maa::ios::remote::ServiceConnection* on_conn = nullptr,
        StartStatus* start_status = nullptr);

    /// 析构释放本地套接字，不自动调用 stopAll；设备会话的停止或续期由上层负责。
    ~StreamSession();

    /// 接收一个 UDP 数据报原文，可能是 RTP 或 RTCP；timeout_ms 为本次接收等待时限。
    /// 协议分类与媒体拆包由调用方完成。
    bool next_packet(std::vector<uint8_t>& packet, int timeout_ms, std::string& err);
    /// 同上，并返回该数据报的源端口，供调用方识别对端和确定反馈目的地。
    bool next_packet(std::vector<uint8_t>& packet, uint16_t& peer_port, int timeout_ms, std::string& err);

    /// 向隧道对端指定端口发送原始 UDP 载荷，用于 RTCP 反馈。
    /// 不添加 RTP 头；调用方负责完整的 RTCP 编码、SSRC 和目的端口。
    bool send_rtp(const std::vector<uint8_t>& payload, uint16_t peer_port, std::string& err);

    /// 通过新服务连接发送 stopmediastream，当前入参固定为 stopAll: XPC Bool true。
    /// 该操作停止设备上所有媒体会话，包括音频和视频，不只停止本对象对应的会话。
    /// 回复 stoppedStreams 中的编号对应 offer 的 u32 session_id，而非 ClientSessionID UUID。
    bool stop(maa::ios::remote::Device& device, std::string& err, bool verbose = false) const;

    [[nodiscard]] uint16_t receiver_port() const;

    [[nodiscard]] const Started& started() const { return started_; }

    /// 以设备 sessions 列表判断会话是否仍存在。
    /// 媒体暂时无包不等于会话结束，例如静止画面可能不产生新的视频帧；
    /// Alive 也不保证正在收包或已成功解码，上层需结合媒体与恢复状态判断。
    enum class ServerState
    {
        /// 设备 sessions 列表中存在匹配 ClientSessionID 的会话。
        Alive,
        /// 已识别的 sessions 列表中未找到匹配 UUID，由上层决定是否重新协商。
        Ended,
        /// 查询失败或回复缺少可识别的 sessions 列表；不能等同于 Ended。
        Unknown,
    };

    /// 通过新连接查询 getmediastreamserverstatus，并按 session_uuid 匹配 ClientSessionID。
    /// 多个媒体流共用 UUID 时只能判断该组是否仍有会话，不能确定某条流的存活状态。
    [[nodiscard]] static ServerState
        probe(remote::Device& device, const std::vector<uint8_t>& session_uuid, std::string& err, bool verbose = false);

    /// 通过新连接返回 getmediastreamserverstatus 的完整输出，失败时返回 Null。
    /// 除会话匹配外，回复中的设备计数器可用于检查 RTCP 是否实际到达等传输问题。
    [[nodiscard]] static maa::ios::xpc::Value status(remote::Device& device, std::string& err, bool verbose = false);

private:
    StreamSession(std::unique_ptr<maa::ios::net::UdpSocket> sock, Started started)
        : socket_(std::move(sock))
        , started_(std::move(started))
    {
    }

    std::unique_ptr<maa::ios::net::UdpSocket> socket_;
    Started started_;
};

/// 将 startmediastream 回复解析为会话参数，不进行网络 I/O。
/// answer 与本次请求的 session_uuid 原样保留；PT、SSRC 维持既有转换和缺失默认值。
/// 仅收紧显式 connection.sender.port：接受 1..65535 的 Int64、UInt64 或完整十进制
/// 字符串，拒绝其它类型、符号、空白、尾随内容及溢出。字段缺失时仍保留 0。
/// 失败返回 nullopt 并说明端口约束，成功清空 err。
/// 本解析器处理 feature 成功后的答复，start_status 非空时失败写 AcceptedInvalidAnswer、
/// 成功写 Started。它不判断或执行 feature RPC。
[[nodiscard]] std::optional<StreamSession::Started> parse_start_answer(
    maa::ios::xpc::Value answer,
    std::vector<uint8_t> session_uuid,
    std::string& err,
    StreamSession::StartStatus* start_status = nullptr);

/// 组装 startmediastream 的 CoreDevice.input，不进行网络 I/O，供起流和离线协议校验使用。
/// options 的参数按 int / string / uuid 标签包装；会话与事件通道 UUID 使用 16 字节
/// XPC UUID 对象，不能用 UUID 文本代替。offer_bytes 原样放入 XPC Data，不转换 plist 格式。
/// audio 决定请求类型及是否省略显示器 options；空共享 UUID 会生成本次 ClientSessionID，
/// 空事件通道 UUID 和 nullopt timeout 分别省略对应键。省略 timeout 在已测设备上被拒。
[[nodiscard]] maa::ios::xpc::Value build_start_request(
    const std::string& receiver_ip,
    uint16_t receiver_port,
    const std::string& sender_ip,
    const std::vector<uint8_t>& offer_bytes,
    uint32_t display_id,
    std::optional<uint32_t> timeout_seconds,
    uint64_t client_supported_features,
    const std::vector<uint8_t>& event_channel_uuid,
    bool audio = false,
    const std::vector<uint8_t>& shared_client_session_uuid = { });

} // namespace maa::ios::media
