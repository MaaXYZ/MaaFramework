#include "media/FramePump.h"

#include "MaaUtils/Logger.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <optional>
#include <utility>

#include "bitstream/AnnexB.h"
#include "media/StreamSession.h"
#include "remote/Device.h"
#include "rt/Rtcp.h"
#include "rt/RtpTimestamp.h"

namespace maa::ios::media
{
namespace
{

/// 设备 SR 的典型周期为一秒，画面静止时也曾观察到持续发送。
/// 作为静默阈值的基准，但缺少 SR 本身不证明会话已经终止。
constexpr uint64_t kSrPeriodMs = 1000;
/// 超过一个 SR 周期加余量后查询设备状态，区分会话结束与 UDP 丢失。
constexpr uint64_t kQuietSuspiciousMs = kSrPeriodMs + 200;
/// 超过两个 SR 周期加余量后直接重建，减少继续查询造成的恢复延迟。
constexpr uint64_t kQuietCertainMs = 2 * kSrPeriodMs + 500;
/// 连续三次因帧过大重建后降级，避免对后端持续不支持的画面频繁建立会话。
/// 实测复杂画面的 IDR 可达 256278 字节，重建并不保证帧尺寸变小。
constexpr int kMaxOversizedRestarts = 3;
/// 降级期间每分钟重试，允许画面复杂度降低后恢复；成功解码后回到正常策略。
constexpr uint64_t kOversizedRetryMs = 60000;
/// 启动五秒后仍无关键帧输出则重建，处理首个 IDR 丢失或解码无输出。
constexpr uint64_t kNokeyBlindMs = 5000;
/// 无关键帧的快速重试上限。达到后转为降级及定时重试，不能永久停止；
/// SR 持续到达时，静默恢复无法处理这种情况。
constexpr int kMaxNokeyRestarts = 3;
/// startmediastream 请求的 timeout 决定 answer 的 RTCPTimeoutInterval。
/// 设备从上次收到有效 RTCP 开始计时；周期 RR 可延长会话，不是启动后的硬到期。
/// 采用 20 秒，兼顾异常退出后设备会话占用的释放时间。修复 UDP 封包后，
/// 真机在每秒 RR 下运行超过 40 秒；无反馈约 20 秒结束。实验表见
/// docs/coredevice.md §13，较大的 timeout 仅延长无反馈时的观察窗口。
constexpr uint32_t kSessionLeaseSeconds = 20;
/// 每秒发送 RR，沿用已验证的视频反馈频率，较 20 秒租期留有余量。
constexpr uint64_t kRtcpPeriodMs = 1000;
/// 等待关键帧期间每秒重发 PLI，限制请求频率。真机曾在 20–35 ms 内
/// 响应 IDR；未恢复时由 stall_restart_ms 的后备会话重建处理。
constexpr uint64_t kPliPeriodMs = 1000;
/// 静默判断使用最后一个数据报而非最后一个视频包，画面不变时 SR 仍可能
/// 到达。可疑区间查询设备，避免仅凭旧视频帧误判连接。

uint64_t now_ms()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

/// 返回 AU 内最大 NAL 的长度，用于判断后端长度上限。
size_t largest_nal(const std::vector<Nal>& au)
{
    size_t m = 0;
    for (const auto& n : au) {
        m = std::max(m, n.size());
    }
    return m;
}

/// 软件解码不可用的提示只输出一次，避免每次重建重复提示同一构建条件。
void warn_no_software()
{
    static std::once_flag warned;
    std::call_once(warned, [] {
        LogWarn << "Software decoder (libavcodec) is not included; continuing with the platform decoder. The current adapter uses 2-byte "
                   "NAL lengths and cannot process larger NALs. Such access units are dropped and the session is recreated, which may "
                   "interrupt video.\nInstall FFmpeg development packages and reconfigure the build.";
    });
}

} // namespace

NokeyAction plan_nokey(const int restarts, const int max_restarts, const bool already_unusable)
{
    if (restarts < max_restarts) {
        return NokeyAction::kRetry;
    }
    // 达到上限后首次标记降级，已降级则等待统一退避计时。
    return already_unusable ? NokeyAction::kWait : NokeyAction::kDegrade;
}

FramePump::FramePump(remote::Device& device, Options options, bool verbose)
    : device_(device)
    , options_(std::move(options))
    , verbose_(verbose)
{
}

std::unique_ptr<FramePump> FramePump::start(remote::Device& device, const Options& options, std::string& err, bool verbose)
{
    auto pump = std::unique_ptr<FramePump>(new FramePump(device, options, verbose));

    // 缺少解码后端时不建立设备媒体会话，避免占用设备资源。
    // 使用编译期能力常量，无需创建平台解码会话试探。
    if (!maa::ios::kHaveDecoder) {
        err = maa::ios::kNoDecoderMessage;
        return nullptr;
    }

    // Create the worker only after initializing the session and recovery clocks.
    if (!pump->restart(err)) {
        return nullptr;
    }

    // 首次关键帧尚未到达时不立即判定停顿；last_packet_ms_ 由 restart() 设置。
    pump->last_decoded_keyframe_ms_ = now_ms();

    pump->worker_running_ = true;
    pump->worker_ = std::thread(&FramePump::loop, pump.get());
    return pump;
}

FramePump::~FramePump()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
    // 先 join worker，再停止设备媒体会话；worker 可能调用 restart() 并修改
    // session_。正常退出显式停流，避免设备继续向无人接收的端口编码，
    // 也避免会话占用影响下一次启动。异常退出仍依赖租期释放。
    if (session_ != nullptr) {
        std::string stop_err;
        if (!session_->stop(device_, stop_err, verbose_)) {
            LogWarn << std::format("Failed to stop device video session (device will release it when the lease expires): {}", stop_err);
        }
        session_.reset();
    }
}

bool FramePump::restart(std::string& err)
{
    /// 开始重建时标记恢复中，新会话首帧输出后清除，供调用方区分恢复和静止。
    reviving_ = true;
    {
        // 先撤销旧会话信息，再做耗时的停流和起流 RPC；网络 I/O 不持有此锁。
        std::lock_guard<std::mutex> lock(mutex_);
        ready_ = false;
        payload_type_ = 0;
        receiver_port_ = 0;
    }

    if (session_ != nullptr) {
        std::string stop_err;
        if (!session_->stop(device_, stop_err, verbose_)) {
            // 停止旧会话失败仍尝试新会话，保留错误供排查设备端残留状态。
            LogWarn << std::format("Failed to stop previous video session (still attempting restart): {}", stop_err);
        }
        session_.reset();
    }

    StreamSession::Request request;
    request.display_id = options_.display_id;
    request.offer = options_.offer;
    // 请求租期使用统一常量，保证保活策略与设备执行的租期一致。
    request.timeout_seconds = kSessionLeaseSeconds;
    StreamSession::StartStatus start_status = StreamSession::StartStatus::NotConfirmed;
    session_ = StreamSession::start(device_, request, err, verbose_, nullptr, &start_status);
    if (session_ == nullptr) {
        if (start_status == StreamSession::StartStatus::AcceptedInvalidAnswer) {
            // 设备可能已经起流；再次请求会产生无人接收的会话。停止视频接收和
            // 续期，等待本次租期释放，不能用 stopAll 中断可能仍在播放的音频。
            negotiation_invalid_ = true;
            video_unusable_ = true;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (terminal_error_.empty()) {
                    terminal_error_ = err;
                }
            }
            cv_.notify_all();
            if (worker_running_) {
                LogError << std::format("Video negotiation failed during recovery; stopping video reception: {}", err);
            }
        }
        reviving_ = false; // 重建失败，结束本次恢复标记。
        return false;
    }
    last_packet_ms_ = now_ms();

    {
        // 会话创建成功后一次发布两个标量。session_ 仍只由起流线程/worker 使用，
        // getter 无需借用其生命周期，也不会在 RPC 期间阻塞。
        std::lock_guard<std::mutex> lock(mutex_);
        payload_type_ = session_->started().payload_type;
        receiver_port_ = session_->receiver_port();
    }

    // 首次启动时 worker 尚未运行，不计入重建次数，也不输出重建提示。
    // 线程创建由 start() 统一管理。
    if (!worker_running_) {
        return true;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++stats_.restarts;
    }
    LogInfo << std::format("Video session recreated, receive port={}", session_->receiver_port());
    return true;
}

void FramePump::loop()
{
    std::string err;
    std::vector<uint8_t> datagram;

    /// 软件解码选择跨会话保留，避免每次重建重复尝试能力不足的硬件后端。
    bool software_only = false;
    std::unique_ptr<Decoder> decoder;
    if (!options_.use_hardware) {
        decoder = create_software_decoder();
        software_only = decoder != nullptr;
        if (!software_only) {
            warn_no_software();
        }
    }
    if (decoder == nullptr) {
        decoder = create_platform_decoder();
    }
    // 两种后端都不可用时退出，避免解引用空 decoder。start() 通常已通过
    // 编译期能力检查拦截，此处仍保留防御检查。
    if (decoder == nullptr) {
        LogError << maa::ios::kNoDecoderMessage;
        return;
    }
    bool configured = false;
    std::unique_ptr<maa::ios::rt::HevcRtpDepacketizer> depacketizer;
    maa::ios::rt::RtpTimestamp sampling_clock;
    // 0 是合法 SSRC，不能用其数值区分缺失。没有协商来源时只由合法的视频 RTP 绑定。
    std::optional<uint32_t> media_source;
    /// 探针用视频接收计数，不包含 SR。
    uint64_t video_seen = 0;

    /// 丢包后优先用 12 字节 PLI 请求 IDR，再等待完整关键帧解码；重建作为后备。
    /// 真机 PLI 响应曾为 20–35 ms，重建会话 RPC 约 37–90 ms，另需等待首帧。
    /// 产品继续使用已验证的 RR 保活和 PLI 恢复。早期错误构造的 FIR
    /// 不能证明标准 FIR 不可用；标准 FIR 对照及验证范围见 docs/coredevice.md §30.3。
    auto request_keyframe = [&] {
        // 在所有提前返回之前记录本轮等待起点，包括探针禁止 PLI 的情况。
        // 后备超时从首次请求计算，不能使用每秒重发都会更新的 last_pli_ms_。
        if (first_pli_ms_ == 0) {
            first_pli_ms_ = now_ms();
        }
        if (!media_source) {
            return; // 尚无来源可作为 PLI 目标，不构造猜测的 0 SSRC。
        }
        if (options_.debug_suppress_pli || pli_off_) {
            // 测试模式可禁止 PLI，用来单独验证等待超时后的会话重建路径。
            std::lock_guard<std::mutex> lock(mutex_);
            ++stats_.pli_suppressed;
            return;
        }
        if (now_ms() < next_pli_ms_) {
            return; // 限制重复关键帧请求的频率。
        }
        next_pli_ms_ = now_ms() + kPliPeriodMs;
        last_pli_ms_ = now_ms();
        const auto pli = maa::ios::rt::build_pli(session_->started().remote_ssrc, *media_source);
        std::string serr;
        if (session_->send_rtp(pli, session_->started().sender_port, serr)) {
            std::lock_guard<std::mutex> lock(mutex_);
            ++stats_.pli_sent;
        }
        else {
            // 发送失败保留诊断；后续静默与关键帧等待策略负责恢复。
            LogWarn << std::format("Failed to send PLI: {}", serr);
        }
    };

    auto discard_for_recovery = [&] {
        parser_->discard_pending();
        awaiting_idr_from_loss_ = true;
        need_keyframe_ = true;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            ready_ = false;
        }
        request_keyframe();
    };

    /// 每次拆包后立即检查序号缺口、作废分片和已匹配 RTP 内的载荷损坏，
    /// 先丢弃未提交 AU，再启动关键帧等待与后备计时。
    /// 不能只在 AU 回调检查：丢包后可能只剩 SR，不再产生 AU。
    auto note_loss = [&] {
        if (depacketizer == nullptr) {
            return;
        }
        const auto& dst = depacketizer->stats();
        const uint64_t loss_now = dst.seq_gaps + dst.dropped_fragments + dst.malformed;
        if (loss_now <= loss_seen_) {
            return;
        }
        loss_seen_ = loss_now;

        if (!need_keyframe_) {
            // 仅在进入关键帧等待状态时输出一次。
            LogWarn << std::format(
                "Video packet loss or payload damage detected: requesting keyframe, wait limit {} ms",
                options_.stall_restart_ms);
        }
        discard_for_recovery();
    };

    // 每个会话重新创建解析器，避免将旧会话未完成的 NAL 或 AU 拼入新会话。
    auto make_parser = [&]() {
        return std::make_unique<AnnexBParser>([&](AnnexBParser::AccessUnit&& unit) {
            auto& au = unit.nals;
            const bool keyframe = unit.keyframe;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                ++stats_.aus;
            }
            // NAL 超过平台后端的长度上限时尝试软件解码。实测 IDR 可达
            // 49652–70101 字节，转场中的非关键帧切片也曾达到 256278 字节。
            const size_t biggest = largest_nal(au);
            if (biggest > decoder->max_nal_size()) {
                // 仅在关键帧切换后端，新解码器尚无旧参考帧。超大非关键帧只记录
                // 后续使用软件解码，并通过重建会话取得用于初始化的 IDR。
                software_only = true;
                if (keyframe) {
                    configured = false;
                }
            }
            if (!configured) {
                if (software_only && decoder->max_nal_size() != ~size_t { 0 }) {
                    auto soft = create_software_decoder();
                    if (soft != nullptr) {
                        decoder = std::move(soft);
                        LogInfo << std::format("Decoder switched to {}", decoder->backend_name());
                    }
                    else {
                        // 软件后端未编入时保留平台后端并提示构建条件。
                        warn_no_software();
                        software_only = false;
                    }
                }
                // 参数前缀可能随受损 AU 被丢弃，但完整参数 NAL 已缓存。
                // 优先保留本 AU 原文；缺少的参数才使用当前会话缓存。
                Nal vps = parser_->vps(), sps = parser_->sps(), pps = parser_->pps();
                for (const auto& n : au) {
                    if (n.size() < 2) {
                        continue;
                    }
                    switch ((n[0] >> 1) & 0x3F) {
                    case 32:
                        vps = n;
                        break;
                    case 33:
                        sps = n;
                        break;
                    case 34:
                        pps = n;
                        break;
                    default:
                        break;
                    }
                }
                if (vps.empty() || sps.empty() || pps.empty() || !decoder->configure(vps, sps, pps)) {
                    return;
                }
                configured = true;
            }
            // 当前后端仍不支持此 NAL 尺寸时丢弃整个 AU。
            if (biggest > decoder->max_nal_size()) {
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    ++stats_.dropped_oversized;
                    // 丢弃 AU 可能破坏参考链，暂停解码后续非关键帧，保留最近的有效画面。
                    need_keyframe_ = true;
                }
                // 首次 IDR 也可能超过上限，即使尚无成功解码记录仍需启动恢复。
                if (oversized_restarts_ >= kMaxOversizedRestarts) {
                    if (!video_unusable_) {
                        video_unusable_ = true;
                        LogWarn << std::format(
                            "After {} restarts, NALs still exceed the decoder limit. Video is unavailable; retrying every {} seconds. "
                            "Callers can use screenshots.",
                            oversized_restarts_,
                            kOversizedRetryMs / 1000);
                    }
                    // 达到重建上限后交由主循环统一管理降级重试，避免重复调度。
                    return;
                }
                // 尚未达到上限时限制重建频率。
                if (now_ms() - last_restart_ms_ > 1500) {
                    last_restart_ms_ = now_ms();
                    ++oversized_restarts_;
                    oversized_restart_ = true;
                }
                return;
            }
            // 受损 AU 已在 note_loss() 中丢弃；此后只能由新 first-slice 开启图像。
            // 后续完整关键帧可直接恢复，不能再用旧的损失标记额外丢掉它。
            // 保存进入回调时的等待状态，成功解码并取得像素后才解除。
            const bool was_awaiting = need_keyframe_;
            // 故障注入使用进入回调时的快照，避免测试触发条件依赖随后被修改的
            // 恢复状态。否则提前清除状态会使注入失效，无法验证恢复逻辑。
            const bool awaiting_at_entry = awaiting_idr_from_loss_;
            if (need_keyframe_) {
                if (!keyframe) {
                    // 非关键帧不能修复参考链，继续请求关键帧。
                    request_keyframe();
                    std::lock_guard<std::mutex> lock(mutex_);
                    ++stats_.dropped_awaiting_keyframe;
                    return;
                }
            }

            // 保留 publishing_.pixels 的容量和尺寸。同尺寸时由解码器直接覆写，
            // 避免 clear() 后 resize() 对整个图像缓冲区重新填零。
            Frame& f = publishing_;
            const uint64_t t_decode0 = now_ms();
            bool ok = false;
            // 默认仅向丢包恢复期间的关键帧注入失败，验证解码无输出后仍会继续
            // 等待并触发恢复。debug_fail_any_keyframe 可扩大到启动阶段。
            if (keyframe && (awaiting_at_entry || options_.debug_fail_any_keyframe) && options_.debug_fail_decode_of_keyframe > 0) {
                --options_.debug_fail_decode_of_keyframe;
                // 可同时禁止后续 PLI，排除新关键帧请求对会话重建测试的影响。
                if (options_.debug_suppress_pli_after_fail) {
                    pli_off_ = true;
                }
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    ++stats_.forced_decode_failures;
                }
                LogDebug << std::format(
                    "Test: injected keyframe decode failure ({})",
                    options_.debug_suppress_pli_after_fail ? "further PLI disabled" : "PLI continues");
            }
            else {
                ok = decoder->decode(au, f);
            }
            const uint64_t t_published0 = now_ms();
            {
                std::lock_guard<std::mutex> lock(mutex_);
                stats_.ms_decode += static_cast<double>(t_published0 - t_decode0);
                ++stats_.decode_calls;
            }
            if (!ok || !f) {
                std::lock_guard<std::mutex> lock(mutex_);
                ++stats_.no_output;
                return;
            }
            // 仅在解码成功且输出有效像素后解除等待。解析出完整关键帧并不保证
            // 解码器输出图像，过早清除会丢失 first_pli_ms_ 对应的后备恢复计时。
            if (was_awaiting) {
                need_keyframe_ = false;
                awaiting_idr_from_loss_ = false;
                first_pli_ms_ = 0;
            }
            if (keyframe) {
                // 收到关键帧但未输出图像不能延后恢复期限。
                last_decoded_keyframe_ms_ = now_ms();
                ever_keyframe_ = true;
                nokey_restarts_ = 0; // 成功输出关键帧后重置快速重试计数。
                // 成功解码后退出降级并重置尺寸限制重试计数。
                oversized_restarts_ = 0;
                video_unusable_ = false;
            }
            f.geometry = options_.frame_geometry ? options_.frame_geometry() : std::nullopt;
            std::lock_guard<std::mutex> lock(mutex_);
            reviving_ = false; // 新会话已输出第一帧，结束恢复等待。
            // 交换两个帧缓冲区，下一次解码复用上一帧的内存，避免 move 后
            // 解码目标失去缓冲区并重新分配。尺寸变化仍由解码器处理。
            std::swap(frame_, publishing_);
            width_ = static_cast<int>(frame_.width);
            height_ = static_cast<int>(frame_.height);
            ++serial_;
            ready_ = true;
            ++stats_.decoded;
            cv_.notify_all();
            stats_.ms_publish += static_cast<double>(now_ms() - t_published0);
        });
    };

    /// 重建后重置解析器、计时器及丢包标记，避免旧状态导致新会话首个
    /// 关键帧被误判为不完整。跨会话的重试策略与测试选项另行保留。
    auto new_session_state = [&] {
        depacketizer = std::make_unique<maa::ios::rt::HevcRtpDepacketizer>(session_->started().payload_type);
        sampling_clock.reset();
        media_source = session_->started().has_local_ssrc ? std::optional<uint32_t>(session_->started().local_ssrc) : std::nullopt;
        configured = false;
        parser_ = make_parser();
        session_start_ms_ = now_ms();
        last_packet_ms_ = session_start_ms_;
        // 新会话立即发送第一次 RR，此后按周期续期。
        next_rtcp_ms_ = session_start_ms_;
        // 重置关键帧请求节流与等待状态，使后续首次丢包可以立即请求 PLI。
        next_pli_ms_ = 0;
        last_pli_ms_ = 0;
        first_pli_ms_ = 0;
        awaiting_idr_from_loss_ = false;
        ever_keyframe_ = false;
        need_keyframe_ = false;
        loss_seen_ = 0;
        // 测试禁止 PLI 的选项跨会话保留，继续隔离会话重建恢复路径。
        // 视频故障注入的计数则按会话归零，避免新会话也被持续忽略。
        video_seen = 0;
        // SR 中的设备视频包计数按会话归零，本地数据报总数跨会话累计。
        // 保存本地会话起点并清空旧 SR 读数，避免新 SR 到达前显示旧设备计数。
        // 两者统计对象不同：本地含 RTCP，设备只报视频，不能直接相减估计丢包。
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stats_.session_packets_base = stats_.packets;
            stats_.dev_sent_packets = 0;
            stats_.dev_sent_octets = 0;
            stats_.other_payload = 0;
            stats_.gaps = 0;
            stats_.dropped_fragments = 0;
        }
    };
    new_session_state();

    /// 可疑静默时查询设备会话状态；已结束或查询失败则尝试重建。
    /// why 和 quiet_ms 仅用于说明触发来源与已等待时间。
    auto revive_if_dead = [&](const char* why, uint64_t quiet_ms) {
        // 查询前标记恢复中，避免调用方在耗时 RPC 期间将旧帧当作已恢复输出。
        reviving_ = true;
        std::string perr;
        const auto state = StreamSession::probe(device_, session_->started().session_uuid, perr, verbose_);
        if (state == StreamSession::ServerState::Alive) {
            // Alive 只证明设备会话存在，不能证明解码输出已恢复。
            if (ever_keyframe_ && !need_keyframe_) {
                reviving_ = false;
            }
            if (verbose_) {
                LogDebug << std::format("{}: no data for {} ms; device reports session alive, continuing to wait", why, quiet_ms);
            }
            return false; // 设备报告会话仍运行，暂不重建。
        }
        LogWarn << std::format(
            "{}: no data for {} ms, {}; recreating video session",
            why,
            quiet_ms,
            state == StreamSession::ServerState::Ended ? "device session ended" : ("session status query failed (" + perr + ")"));
        std::string restart_err;
        if (!restart(restart_err)) {
            if (!negotiation_invalid_) {
                LogWarn << std::format("Failed to recreate video session: {}", restart_err);
            }
            reviving_ = false;
            return false;
        }
        new_session_state();
        return true;
    };

    /// 直接重建会话并重置收流状态，省去状态查询 RPC 的等待。
    /// 用于超过静默阈值或需要重新取得关键帧的情况。
    auto restart_now = [&]() {
        std::string restart_err;
        if (!restart(restart_err)) {
            if (!negotiation_invalid_) {
                LogWarn << std::format("Failed to recreate video session: {}", restart_err);
            }
            return false;
        }
        new_session_state();
        return true;
    };

    /// 按最后一个数据报的时间处理静默，包括 RTCP SR：
    /// 静默不超过 1.2 秒时继续等待；介于 1.2 秒和 blind_at 之间时查询设备；
    /// 超过 blind_at 时直接重建。缺少 SR 可能是丢包，不能单独证明会话结束。
    /// ask_every_ms 限制中间区间的查询频率；用户 wake 传 0，不额外节流。
    uint64_t last_ask_ms_ = 0;
    auto judge_quiet = [&](uint64_t quiet, uint64_t blind_at, uint64_t ask_every_ms, const char* why) {
        if (quiet > blind_at) {
            LogWarn << std::format("{}: no data for {} ms; recovery threshold exceeded, recreating video session", why, quiet);
            restart_now();
        }
        else if (quiet > kQuietSuspiciousMs) {
            const uint64_t now = now_ms();
            if (ask_every_ms == 0 || now - last_ask_ms_ >= ask_every_ms) {
                last_ask_ms_ = now;
                revive_if_dead(why, quiet);
            }
        }
    };

    for (;;) {
        // 已接受但不可用的协商结果不再重试。其设备会话不续期，等待租期释放。
        if (negotiation_invalid_) {
            return;
        }
        // 普通 restart() 失败会使 session_ 为空。统一在此重试并跳过后续会话访问；
        // 通过可取消的条件变量等待实现一秒退避，使停止请求能够及时退出。
        if (session_ == nullptr) {
            std::string rerr;
            if (restart(rerr)) {
                new_session_state();
                continue;
            }
            if (negotiation_invalid_) {
                return;
            }
            LogWarn << std::format("Failed to recreate video session: {} (retry in 1 second)", rerr);
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::seconds(1), [this] { return stopping_; });
            if (stopping_) {
                return;
            }
            continue;
        }
        // 每秒发送 RR，延长设备 RTCPTimeoutInterval 定义的空闲租期。
        // 调度置于每轮循环，不能依赖读包超时，否则连续视频包会阻止发送。
        // 沿用真机验证的 32 字节裸 RR：发送者为 answer.RemoteSSRC，
        // 报告块指向设备 LocalSSRC，发至 sender.port（已验证与 SourcePort 一致）。
        if (!options_.debug_suppress_rr && media_source && now_ms() >= next_rtcp_ms_) {
            next_rtcp_ms_ += kRtcpPeriodMs;
            const auto rr = maa::ios::rt::build_rr(
                session_->started().remote_ssrc,
                *media_source,
                depacketizer ? depacketizer->extended_sequence() : 0);
            std::string serr;
            const bool ok = session_->send_rtp(rr, session_->started().sender_port, serr);
            uint64_t failed_after = 0;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (ok) {
                    ++stats_.rtcp_sent;
                }
                else {
                    failed_after = ++stats_.rtcp_failed;
                }
            }
            if (failed_after == 1) {
                // 每个会话只输出第一次发送失败，避免重复日志掩盖恢复结果。
                LogWarn << std::format("Failed to send RTCP keepalive: {}", serr);
            }
        }
        // 收到取帧或输入触发的 wake 请求后，按相同静默策略检查会话。
        if (wake_requested_.exchange(false)) {
            judge_quiet(now_ms() - last_packet_ms_, kQuietCertainMs, 0, "Input or frame request");
            continue;
        }
        // 降级重试计时每轮检查，不依赖收到超大 AU 或发生读超时。
        // 持续收包和完全无包两种情况都必须能执行定时恢复。
        if (video_unusable_ && now_ms() - last_restart_ms_ >= kOversizedRetryMs) {
            last_restart_ms_ = now_ms();
            oversized_restart_ = true;
            LogWarn << std::format("Video fallback active for {} seconds; retrying video session", kOversizedRetryMs / 1000);
        }
        if (oversized_restart_) {
            oversized_restart_ = false;
            // 降级既可能来自尺寸限制，也可能来自连续无关键帧输出，日志不能
            // 将定时重试一律归因于后端尺寸限制。
            LogWarn << std::format(
                "{}; recreating video session to obtain a keyframe",
                video_unusable_ ? "Scheduled video retry after fallback" : "NAL exceeds platform decoder length limit");
            // 失败后的空会话由循环顶部统一退避重试。
            restart_now();
            continue;
        }
        // PLI 后的后备重建每轮检查，即使只收到 SR 而没有视频字节也要推进
        // 等待计时。条件同时要求丢包后的等待未结束及关键帧等待达到期限。
        // PLI 重发按时间调度，不能依赖新的 AU。只剩 SR 时也需要继续请求。
        if (awaiting_idr_from_loss_) {
            request_keyframe();
        }
        if (options_.stall_restart_ms > 0 && depacketizer != nullptr) {
            const auto& dst = depacketizer->stats();
            const uint64_t gaps = dst.seq_gaps;
            bool stalled = false;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                // awaiting_idr_from_loss_ 表示丢包后的等待，成功解码关键帧才清除；
                // first_pli_ms_ 是首次请求时间，重发不延后期限；last_decoded_keyframe_ms_
                // 记录最近成功输出关键帧的时间，收到但解码失败的 AU 不延后期限。
                // 不能以本轮新增缺口代替持续等待：缺口发生时超时尚未到达，
                // 期限到达时又不再有新缺口，会使两项条件永远无法同时成立。
                stalled = awaiting_idr_from_loss_ && first_pli_ms_ != 0
                          && now_ms() - first_pli_ms_ >= static_cast<uint64_t>(options_.stall_restart_ms)
                          && now_ms() - last_decoded_keyframe_ms_ > static_cast<uint64_t>(options_.stall_restart_ms);
                stats_.gaps = gaps;
                stats_.dropped_fragments = dst.dropped_fragments;
                if (stalled) {
                    last_decoded_keyframe_ms_ = now_ms(); // 为新会话留出等待时间。
                }
            }
            if (stalled) {
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    ++stats_.stall_restarts;
                }
                LogWarn << std::format("Keyframe wait reached {} ms; recreating video session", options_.stall_restart_ms);
                restart_now(); // 失败由循环顶端的空会话兜底接手退避重试
                continue;      // 这一轮的包属于上一条会话了
            }
        }

        // 启动后无关键帧输出时按 plan_nokey 执行有限快速重试，再转定时重试。
        // 此计时每轮检查，不能只放在 next_packet 超时分支：连续收包但解码
        // 无输出时仍需要恢复。实测回归过程见 docs/coredevice.md §20。
        if (!ever_keyframe_ && now_ms() - session_start_ms_ > kNokeyBlindMs) {
            switch (plan_nokey(nokey_restarts_, kMaxNokeyRestarts, video_unusable_)) {
            case NokeyAction::kRetry:
                ++nokey_restarts_;
                session_start_ms_ = now_ms();
                LogWarn << std::format(
                    "No keyframe output after {} seconds; recreating video session ({}/{})",
                    kNokeyBlindMs / 1000,
                    nokey_restarts_,
                    kMaxNokeyRestarts);
                restart_now();
                continue; // 这一轮的包属于上一条会话了
            case NokeyAction::kDegrade:
                video_unusable_ = true;
                LogWarn << std::format(
                    "No keyframe output after {} restarts. Video is unavailable; callers can use screenshots. Retrying every {} seconds.",
                    kMaxNokeyRestarts,
                    kOversizedRetryMs / 1000);
                break;
            case NokeyAction::kWait:
                break;
            }
            // 降级重试由上方 video_unusable_ 计时统一调度。
        }

        if (!session_->next_packet(datagram, 50, err)) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (stopping_) {
                    return;
                }
            }
            // 接收超时或失败时按静默时长判断恢复，不凭一次接收失败认定会话结束。
            if (options_.silence_restart_ms > 0) {
                const uint64_t quiet = now_ms() - last_packet_ms_;
                judge_quiet(
                    quiet,
                    std::max<uint64_t>(kQuietCertainMs, static_cast<uint64_t>(options_.silence_restart_ms)),
                    kSrPeriodMs,
                    "Receive silence timeout");
            }
            continue; // 超时不是结束
        }
        // 视频端口同时接收裸 RTCP SR。按既有设备心跳形态识别后读取偏移 20 和 24
        // 的设备累计视频包数与字节数；这些值只反映最后一份 SR 的时刻。
        const bool is_sr = maa::ios::rt::is_rtcp_sr(datagram);
        const auto be32 = [&datagram](std::size_t off) {
            return (uint32_t(datagram[off]) << 24) | (uint32_t(datagram[off + 1]) << 16) | (uint32_t(datagram[off + 2]) << 8)
                   | datagram[off + 3];
        };
        // Validate compound RTCP before isolating control packets from the RTP depacketizer.
        std::vector<maa::ios::rt::SenderReport> reports;
        const bool valid_rtcp = maa::ios::rt::parse_sender_reports(datagram, reports);

        if (is_sr) {
            // 未协商来源时不能用心跳猜测媒体源，等待第一条合法同 PT 的视频 RTP。
            if (!media_source || be32(4) != *media_source) {
                continue;
            }
        }
        else {
            // 合法的短 RR 等控制包不占用 RTP 序号，也不能让待提交 AU 失效。
            // 保留上述设备心跳分类；这里仅将其他已验证 RTCP 隔离出视频拆包路径。
            if (valid_rtcp) {
                continue;
            }
            maa::ios::rt::PacketInfo info;
            if (!maa::ios::rt::parse_rtp_header(datagram, info) || info.payload_type != session_->started().payload_type) {
                continue;
            }
            if (!media_source) {
                media_source = info.ssrc;
            }
            else if (info.ssrc != *media_source) {
                continue; // 在拆包器观察序号前排除外来源，保持本会话的 FU/ticks 状态。
            }
        }
        last_packet_ms_ = now_ms();
        uint64_t dev_pkts = 0, dev_octets = 0;
        if (is_sr) {
            dev_pkts = be32(20);
            dev_octets = be32(24);
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stopping_) {
                return;
            }
            ++stats_.packets;
            if (is_sr) {
                ++stats_.sr_packets;
                ++stats_.other_payload;
            }
            else {
                ++stats_.video_packets;
            }
            if (dev_pkts != 0) {
                stats_.dev_sent_packets = dev_pkts;
                stats_.dev_sent_octets = dev_octets;
            }
        }

        if (is_sr) {
            continue;
        }

        ++video_seen;
        if (options_.debug_drop_nth_packet > 0 && video_seen == static_cast<uint64_t>(options_.debug_drop_nth_packet)) {
            LogDebug << std::format("Test: dropped video packet {} to inject a sequence gap", options_.debug_drop_nth_packet);
            options_.debug_drop_nth_packet = 0;    // 一次性：重起之后的新会话不该再被丢
            std::lock_guard<std::mutex> lock(mutex_);
            stats_.debug_dropped_at_ms = now_ms(); // 记录故障注入时间，供测试检查恢复期限。
            continue;
        }
        if (options_.debug_ignore_video_after > 0 && video_seen > static_cast<uint64_t>(options_.debug_ignore_video_after)) {
            // 测试模式忽略超过指定计数的视频包，仅保留 SR，用于验证持续收到
            // RTCP 但无视频输出时恢复计时仍能触发。
            continue;
        }
        std::vector<maa::ios::rt::ReceivedNal> nals;
        const uint64_t t_dp0 = now_ms();
        const bool pushed = depacketizer->push_nals(datagram, nals, err);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stats_.gaps = depacketizer->stats().seq_gaps;
            stats_.dropped_fragments = depacketizer->stats().dropped_fragments;
            stats_.ms_depacketize += static_cast<double>(now_ms() - t_dp0);
        }
        note_loss();
        if (!pushed || nals.empty()) {
            continue;
        }
        for (auto& nal : nals) {
            const auto timestamp = sampling_clock.observe(nal.timestamp);
            if (!timestamp) {
                if (!need_keyframe_) {
                    LogWarn << "Video sampling timestamp is ambiguous; discarding the access unit and requesting a keyframe.";
                }
                discard_for_recovery();
                continue;
            }

            if (!parser_->push_nal(std::move(nal.bytes), timestamp, nal.ends_access_unit)) {
                if (!need_keyframe_) {
                    LogWarn << "Video access-unit input is inconsistent; discarding the access unit and requesting a keyframe.";
                }
                discard_for_recovery();
            }
        }
    }
}

bool FramePump::latest(Frame& out, int timeout_ms)
{
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&] { return serial_ > 0 || stopping_; });
    if (serial_ == 0) {
        return false;
    }
    out = frame_;
    return true;
}

bool FramePump::wait_ready(int timeout_ms)
{
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&] { return ready_ || stopping_ || video_unusable_; });
    return ready_ && !stopping_ && !video_unusable_;
}

std::string FramePump::capture_error() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return terminal_error_;
}

uint64_t FramePump::newer(Frame& out, uint64_t since, int timeout_ms)
{
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), [&] { return serial_ > since || stopping_; });
    if (serial_ <= since) {
        return 0;
    }
    out = frame_;
    return serial_;
}

uint64_t FramePump::serial() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return serial_;
}

FramePump::Stats FramePump::stats() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

uint8_t FramePump::payload_type() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return payload_type_;
}

uint16_t FramePump::receiver_port() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return receiver_port_;
}

void FramePump::size(int& width, int& height) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    width = width_;
    height = height_;
}

DisplayCrop display_crop(int coded_w, int coded_h)
{
    if (coded_w == 1136 && coded_h == 2464) {
        return DisplayCrop { 0, 0, 1125, 2436 };
    }
    return DisplayCrop { 0, 0, coded_w, coded_h };
}

} // namespace maa::ios::media
