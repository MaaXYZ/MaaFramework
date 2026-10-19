#include "media/StreamSession.h"

#include <array>
#include <charconv>
#include <limits>
#include <random>

#include "remote/Device.h"

namespace maa::ios::media
{
namespace
{

using namespace maa::ios;

constexpr int64_t kAccessNetworkType = 1;
constexpr int64_t kTransportProtocolType = 2;

/// SSRC 是完整的无符号 32 位值，包括 0。只在范围及实际 XPC 类型均合法时收窄，
/// 避免负数、大 UInt64 或 Bool 被 as_int_or 隐式转换成另一个媒体源。
bool parse_ssrc(const xpc::Value& value, uint32_t& out)
{
    uint64_t source = 0;
    if (value.type == xpc::Type::UInt64) {
        source = value.uint64;
    }
    else if (value.type == xpc::Type::Int64 && value.int64 >= 0) {
        source = static_cast<uint64_t>(value.int64);
    }
    else {
        return false;
    }
    if (source > std::numeric_limits<uint32_t>::max()) {
        return false;
    }
    out = static_cast<uint32_t>(source);
    return true;
}

/// 将 options 参数放入带类型标签的字典：{"int": …}、{"string": …} 或
/// {"uuid": …}。该包装来自已验证的 CoreDevice 请求形状，不能省略这一层。
xpc::Value typed(const char* tag, xpc::Value inner)
{
    auto d = xpc::make_dict();
    xpc::dict_set(d, tag, std::move(inner));
    return d;
}

/// 为本次请求生成 16 字节 ClientSessionID。已测设备的 UUID 类型检查要求 XPC UUID
/// 对象，不能用 UUID 文本代替；该标识与媒体 offer 中的 u32 SSRC 分别处理。
std::array<uint8_t, 16> random_uuid_bytes()
{
    std::array<uint8_t, 16> out { };
    for (auto& byte : out) {
        byte = static_cast<uint8_t>(std::random_device { }());
    }
    return out;
}

/// 从 49152..65151 选择接收端口。音视频工作线程可能同时重新建立会话，使用
/// thread_local 避免共享伪随机数发生器；实际可用性仍由随后 bind() 的结果决定。
uint16_t pick_port()
{
    static thread_local std::mt19937 rng { std::random_device { }() };
    return static_cast<uint16_t>(49152 + rng() % 16000);
}

} // namespace

xpc::Value build_start_request(
    const std::string& receiver_ip,
    uint16_t receiver_port,
    const std::string& sender_ip,
    const std::vector<uint8_t>& offer_bytes,
    uint32_t display_id,
    std::optional<uint32_t> timeout_seconds,
    uint64_t client_supported_features,
    const std::vector<uint8_t>& event_channel_uuid,
    bool audio,
    const std::vector<uint8_t>& shared_client_session_uuid)
{
    auto d = xpc::make_dict();
    xpc::dict_set(d, "clientSupportedFeatures", xpc::make_uint64(client_supported_features));
    xpc::dict_set(d, "direction", xpc::make_string("output"));
    xpc::dict_set(d, "negotiatorOffer", xpc::make_data(offer_bytes));

    auto options = xpc::make_dict();
    xpc::dict_set(options, "AVCMediaStreamNegotiatorAccessNetworkType", typed("int", xpc::make_int64(kAccessNetworkType)));
    xpc::dict_set(options, "AVCMediaStreamNegotiatorTransportProtocolType", typed("int", xpc::make_int64(kTransportProtocolType)));
    // 已验证的音频请求仅包含 AccessNetworkType / TransportProtocolType /
    // ClientSessionID；视频请求另含显示器选择参数。音频请求省略下面两个键，
    // 对应已测 answer 中的 source: {audioSystemOutput: {}}。
    if (!audio) {
        xpc::dict_set(options, "CoreDeviceVideoDisplayMode", typed("string", xpc::make_string("DisplayByID")));
        xpc::dict_set(options, "VideoStreamForDisplayID", typed("int", xpc::make_int64(display_id)));
    }
    // ClientSessionID 必须编码为 16 字节 XPC UUID；已测 Swift Codable 类型错误为
    // "Expected to decode UUID but found a OS_xpc_string instead"（code 4864）。
    //
    // shared_client_session_uuid 非空时原样使用，否则生成本次请求的标识。
    // 各流使用独立 UUID，避免状态查询把另一条流视为本条存活。
    // 共用 UUID 时，probe() 只能确认至少一条匹配会话存在。
    std::vector<uint8_t> session_id = shared_client_session_uuid;
    if (session_id.empty()) {
        const auto fresh = random_uuid_bytes();
        session_id.assign(fresh.begin(), fresh.end());
    }
    xpc::dict_set(options, "avcMediaStreamOptionClientSessionID", typed("uuid", xpc::make_uuid(std::span<const uint8_t>(session_id))));
    xpc::dict_set(d, "options", std::move(options));

    xpc::dict_set(d, "receiverIP", xpc::make_string(receiver_ip));
    xpc::dict_set(d, "receiverPort", xpc::make_uint64(receiver_port));
    xpc::dict_set(d, "senderIP", xpc::make_string(sender_ip));
    // 已测 CoreDevice feature 在省略 timeout 时返回
    // `code 4865 / Expected to find key timeout.`。DeviceHub 请求样本也包含 timeout=20；
    // 设备会话表未回显某个键，不能用于推断原始请求省略了它。
    if (timeout_seconds.has_value()) {
        xpc::dict_set(d, "timeout", xpc::make_uint64(*timeout_seconds));
    }
    xpc::dict_set(d, "type", xpc::make_string(audio ? "audio" : "video"));
    // DeviceHub 请求样本包含 sessionEventChannel。本构造器允许提供该 UUID，
    // 空 vector 时省略；通道注册过程和对端语义尚未确认，不能仅凭此字段判断续期。
    if (!event_channel_uuid.empty()) {
        xpc::dict_set(d, "sessionEventChannel", xpc::make_uuid(std::span<const uint8_t>(event_channel_uuid)));
    }
    return d;
}

std::optional<StreamSession::Started>
    parse_start_answer(xpc::Value answer, std::vector<uint8_t> session_uuid, std::string& err, StreamSession::StartStatus* start_status)
{
    if (start_status != nullptr) {
        *start_status = StreamSession::StartStatus::AcceptedInvalidAnswer;
    }
    StreamSession::Started started;
    started.answer = std::move(answer);
    started.session_uuid = std::move(session_uuid);

    // answer 的 connection.sender.port 是设备媒体发送端口，
    // connection.streamConfig.RxPayloadType 是本次协商的媒体载荷类型。
    const auto* connection = started.answer.find("connection");
    if (connection != nullptr) {
        // 媒体 PT 使用协商结果；缺失时沿用默认 100。RTP 的该字段只有七位，因此
        // 取低七位。音视频的接收端仍需先识别裸 RTCP，再按媒体 PT 处理 RTP。
        if (const auto* sc = connection->find("streamConfig"); sc != nullptr) {
            started.payload_type = static_cast<uint8_t>(sc->at("RxPayloadType").as_int_or(100) & 0x7F);
            // 名称采用设备视角：LocalSSRC 是设备媒体源，客户端 RTCP 报告块引用它；
            // RemoteSSRC 是客户端反馈的发送者身份。已测设备回显 offer 中声明的 SSRC，
            // 不代表总会另行生成新值；反馈使用本次 answer 中的两个值。
            if (const auto* source = sc->find("LocalSSRC"); source != nullptr) {
                if (!parse_ssrc(*source, started.local_ssrc)) {
                    err =
                        "Invalid startmediastream answer: connection.streamConfig.LocalSSRC must "
                        "be an integer in 0..4294967295";
                    return std::nullopt;
                }
                started.has_local_ssrc = true;
            }
            if (const auto* source = sc->find("RemoteSSRC"); source != nullptr) {
                if (!parse_ssrc(*source, started.remote_ssrc)) {
                    err =
                        "Invalid startmediastream answer: connection.streamConfig.RemoteSSRC must "
                        "be an integer in 0..4294967295";
                    return std::nullopt;
                }
                started.has_remote_ssrc = true;
            }
        }
        if (const auto* sender = connection->find("sender"); sender != nullptr) {
            if (const auto* p = sender->find("port"); p != nullptr) {
                // 先按实际 XPC 类型读取，避免 as_int_or 把 Bool 当作端口，或将大的
                // UInt64 转成有符号数。范围检查通过后才收窄为 uint16_t。
                uint64_t port = 0;
                bool parsed = false;
                if (p->type == xpc::Type::Int64 && p->int64 > 0) {
                    port = static_cast<uint64_t>(p->int64);
                    parsed = true;
                }
                else if (p->type == xpc::Type::UInt64) {
                    port = p->uint64;
                    parsed = true;
                }
                else if (p->is_string()) {
                    const char* begin = p->string.data();
                    const char* end = begin + p->string.size();
                    const auto result = std::from_chars(begin, end, port, 10);
                    parsed = result.ec == std::errc { } && result.ptr == end;
                }
                if (!parsed || port == 0 || port > std::numeric_limits<uint16_t>::max()) {
                    err =
                        "Invalid startmediastream answer: connection.sender.port must be an integer "
                        "or decimal string in 1..65535";
                    return std::nullopt;
                }
                started.sender_port = static_cast<uint16_t>(port);
            }
        }
    }
    err.clear();
    if (start_status != nullptr) {
        *start_status = StreamSession::StartStatus::Started;
    }
    return started;
}

std::unique_ptr<StreamSession> StreamSession::start(
    remote::Device& device,
    const Request& request,
    std::string& err,
    bool verbose,
    remote::ServiceConnection* on_conn,
    StartStatus* start_status)
{
    if (start_status != nullptr) {
        *start_status = StartStatus::NotConfirmed;
    }
    const auto info = device.rsd().service("com.apple.coredevice.displayservice");
    if (!info) {
        // 错误中同时列出实际 RSD 目录，便于区分 DDI 状态与设备/系统服务差异。
        err = maa::ios::remote::Rsd::missing_service_message("com.apple.coredevice.displayservice", device.rsd().services());
        return nullptr;
    }

    // 已测设备在起流回复后立即发送 RTP；提前绑定接收端口，避免丢失视频参数集
    // VPS/SPS/PPS 与首个关键帧。音频请求同样先准备接收端口。
    const uint16_t port = request.receiver_port != 0 ? request.receiver_port : pick_port();
    auto socket = std::make_unique<net::UdpSocket>(device.rsd().stack(), port);
    if (!socket->bind(err)) {
        err = "Failed to bind UDP port " + std::to_string(port) + ": " + err;
        return nullptr;
    }

    Offer offer = request.offer;
    offer.is_audio = request.audio;
    offer.session_id = static_cast<uint32_t>(std::random_device { }());
    offer.call_id = remote::random_uuid_text();
    // raw_offer 非空时原样发送，跳过本地 XML offer 构造。该入口用于完整样本重放和
    // binary/XML 对照，不只比较已识别的字段。
    // 样本内的 SSRC/CallID 保持原值，与上面新生成的 Offer 字段无关。后续 RTCP 应
    // 使用本次 answer 的 LocalSSRC/RemoteSSRC，不能采用未进入 raw_offer 的本地身份。
    // 样本重放本身不能排除反馈送达或其他请求参数的影响，验证范围见 Request::raw_offer。
    std::vector<uint8_t> blob = request.raw_offer.empty() ? build_negotiator_offer(offer) : request.raw_offer;

    const auto& tunnel_params = device.tunnel_params();
    const std::vector<uint8_t> event_channel = request.session_event_channel.value_or(std::vector<uint8_t> { });
    auto input = build_start_request(
        tunnel_params.client_address,
        port,
        tunnel_params.server_address,
        blob,
        request.display_id,
        request.timeout_seconds,
        request.client_supported_features,
        event_channel,
        request.audio,
        request.client_session_uuid);
    xpc::Value output;
    if (on_conn != nullptr) {
        // 借用调用方持有的连接，不转移所有权。invoke() 的非 Ok 结果均表示起流失败；
        // 保留其错误信息，只有缺少诊断的传输错误才在此补充说明。
        const auto r = on_conn->invoke(
            "com.apple.coredevice.feature.startmediastream",
            "com.apple.coredevice.action.mediastreamstart",
            input,
            output,
            30000,
            err);
        if (r != remote::CallResult::Ok) {
            if (r == remote::CallResult::TransportError && err.empty()) {
                err = "startmediastream failed on the retained connection (transport failed)";
            }
            return nullptr;
        }
    }
    else if (!device.feature(
                 "com.apple.coredevice.displayservice",
                 "com.apple.coredevice.feature.startmediastream",
                 "com.apple.coredevice.action.mediastreamstart",
                 input,
                 output,
                 err,
                 verbose,
                 30000)) {
        return nullptr;
    }

    // 从实际请求的类型包装中读取 ClientSessionID，保证记录的是本次发送的 UUID，
    // 包括调用方提供共享标识与本地新生成标识两种情况。
    auto started = parse_start_answer(
        std::move(output),
        input.at("options").at("avcMediaStreamOptionClientSessionID").at("uuid").data,
        err,
        start_status);
    if (!started) {
        // RPC 已成功，设备可能已起流。只释放本次局部 socket，不调用会停止其它
        // 会话的 stopAll，也不关闭借用的 on_conn；设备端仍按请求租期处理空闲流。
        return nullptr;
    }
    return std::unique_ptr<StreamSession>(new StreamSession(std::move(socket), std::move(*started)));
}

StreamSession::~StreamSession() = default;

bool StreamSession::next_packet(std::vector<uint8_t>& packet, int timeout_ms, std::string& err)
{
    uint16_t peer_port = 0;
    return next_packet(packet, peer_port, timeout_ms, err);
}

bool StreamSession::next_packet(std::vector<uint8_t>& packet, uint16_t& peer_port, int timeout_ms, std::string& err)
{
    return socket_->recv(packet, peer_port, timeout_ms, err);
}

bool StreamSession::send_rtp(const std::vector<uint8_t>& payload, uint16_t peer_port, std::string& err)
{
    return socket_->send(payload, peer_port, err);
}

bool StreamSession::stop(remote::Device& device, std::string& err, bool verbose) const
{
    // 使用新连接，避免干扰起流连接；stopAll 会停止设备上的所有媒体会话。
    auto input = xpc::make_dict();
    xpc::dict_set(input, "stopAll", xpc::make_bool(true));
    xpc::Value output;
    return device.feature(
        "com.apple.coredevice.displayservice",
        "com.apple.coredevice.feature.stopmediastream",
        "com.apple.coredevice.action.mediastreamstop",
        input,
        output,
        err,
        verbose,
        10000);
}

uint16_t StreamSession::receiver_port() const
{
    return socket_->local_port();
}

xpc::Value StreamSession::status(remote::Device& device, std::string& err, bool verbose)
{
    auto input = xpc::make_dict();
    xpc::Value output;
    if (!device.feature(
            "com.apple.coredevice.displayservice",
            "com.apple.coredevice.feature.getmediastreamserverstatus",
            "com.apple.coredevice.action.mediastreamstatus",
            input,
            output,
            err,
            verbose,
            10000)) {
        return xpc::Value { };
    }
    return output;
}

StreamSession::ServerState
    StreamSession::probe(remote::Device& device, const std::vector<uint8_t>& session_uuid, std::string& err, bool verbose)
{
    const xpc::Value output = status(device, err, verbose);
    if (output.type == xpc::Type::Null) {
        return ServerState::Unknown;
    }
    // 已测状态回复形状：{sessions: [{connection: {options:
    // {avcMediaStreamOptionClientSessionID: {uuid: ...}}, streamConfig: {...}}}],
    // running: false, runDurationSeconds: 0}。
    const auto* sessions = output.find("sessions");
    if (sessions == nullptr || !sessions->is_array()) {
        err = "getmediastreamserverstatus response missing sessions array";
        return ServerState::Unknown;
    }
    for (const auto& s : sessions->array) {
        // 逐层检查匹配路径；缺少连接或 ClientSessionID 的条目不参与匹配。
        // 某条记录缺字段不使整个列表失效，也不能据其他 UUID 判断当前会话存活。
        const auto* options = s.find("connection");
        if (options == nullptr) {
            continue;
        }
        const auto* wrapped = options->at("options").find("avcMediaStreamOptionClientSessionID");
        if (wrapped == nullptr) {
            continue;
        }
        const auto* uuid = wrapped->find("uuid");
        if (uuid != nullptr && uuid->data == session_uuid) {
            return ServerState::Alive;
        }
    }
    return ServerState::Ended;
}

} // namespace maa::ios::media
