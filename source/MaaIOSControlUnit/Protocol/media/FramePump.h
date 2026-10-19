#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include "decode/Decoder.h"
#include "media/MediaOffer.h"
#include "rt/RtpHevc.h"

namespace maa::ios::media
{

class StreamSession;
}

namespace maa::ios::remote
{
class Device;
}

namespace maa::ios::media
{

/// 持续无法解出关键帧时选择等待、重建会话或降级。
/// 达到重试上限后仍需低频重试：SR 可能继续到达，静默恢复不会触发，
/// 而新会话也没有旧的丢包等待状态。此规则独立测试，避免永久停留在旧画面。
enum class NokeyAction
{
    kWait,    ///< 还在盲等，或者已经降级、交给退避计时管
    kRetry,   ///< 还没到上限：照正常节拍重起一次会话
    kDegrade, ///< 到顶了：置降级标记（此后由泵的退避计时继续慢速再试，**不是停手**）
};

/// restarts 为此原因已经重启的次数；already_unusable 表示已进入降级状态。
NokeyAction plan_nokey(int restarts, int max_restarts, bool already_unusable);

/// 后台执行收包、组帧和视频恢复，默认解码供调用方取得最新画面。
/// 本类不承担 HID 认证；设备输入控制可以独立于视频会话运行。
class FramePump
{
public:
    struct Options
    {
        uint32_t display_id = 1;
        // Sample display metadata together with the decoded frame; callers never reinterpret
        // an old cached image using a newer orientation snapshot.
        std::function<std::optional<FrameGeometry>()> frame_geometry;
        /// 丢包后等待干净关键帧的最长时间，0 禁用此项后备重建。
        /// 优先发 PLI 请求 IDR，超时后重建会话。
        int stall_restart_ms = 2000;
        /// 数据报完全停止后的直接重建阈值，0 禁用静默检查。
        /// 序号缺口无法发现完全无包的情况。较短静默（约 1.2 s）先查询设备状态，
        /// 超过此阈值直接重建；与 wake() 共用基于 SR 周期的判断。
        int silence_restart_ms = 3000;
        /// 起流时用的 offer（码率、能力串都在这里）。每次重起沿用同一份。
        Offer offer;
        /// 使用平台硬件后端，macOS 当前为 VideoToolbox。默认软件解码可处理
        /// 超过当前硬件适配 2 字节 NAL 长度上限的帧。
        /// 软件解码消耗更多 CPU，硬件解码遇到过大帧时可能切换后端或丢帧恢复。
        bool use_hardware = false;

        /// 以下 debug 选项仅供探针，生产默认关闭。
        /// 在拆包前丢弃第 N 个视频包，确定性地产生序号缺口；0 禁用，SR 不计入序号。
        int debug_drop_nth_packet = 0;
        /// 禁止发送 PLI。结合丢包注入及禁用静默重建，可单独验证 stall_restart_ms
        /// 等待超时后的恢复路径。
        bool debug_suppress_pli = false;
        /// 第 M 个视频包后忽略视频，但仍处理 SR，用于验证只有心跳时的恢复判断。
        int debug_ignore_video_after = 0;
        /// 将接下来 N 个用于修复丢包的关键帧 AU 判为解码失败，不交给解码器。
        /// 使用进入回调时的等待状态，排除会话首帧，验证恢复标记只能在成功解码后清除。
        int debug_fail_decode_of_keyframe = 0;
        /// 与上项配合时，对所有关键帧注入失败，包括启动首帧；用于验证持续无输出时的降级。
        bool debug_fail_any_keyframe = false;
        /// 首次注入解码失败后禁用后续 PLI，避免下一次正常 IDR 恢复掩盖后备重建路径。
        bool debug_suppress_pli_after_fail = false;
        /// 仅供无反馈到期实验：禁止周期 RR，默认 false 保留生产保活。
        /// 关闭自动重建不会关闭 RR；探针必须单独选择此项。
        bool debug_suppress_rr = false;
    };

    struct Stats
    {
        uint64_t packets = 0;
        uint64_t decoded = 0;
        /// 解了但没出图的 AU 数（参考帧未就绪，或丢了分片）。
        uint64_t no_output = 0;
        uint64_t gaps = 0;
        uint64_t restarts = 0;
        /// 因解码器不可用而丢弃的帧数（取帧方跟不上时不阻塞收包线程）。
        uint64_t dropped = 0;
        /// NAL 超过当前解码后端长度限制而丢弃 AU 的次数；这可能损坏参考链并触发恢复。
        uint64_t dropped_oversized = 0;
        /// 等待干净关键帧时被跳过的 AU 数，用于区分收包与解码输出。
        uint64_t dropped_awaiting_keyframe = 0;
        /// 当前拆包器作废未完成分片的次数，与序号缺口共同触发关键帧等待。
        uint64_t dropped_fragments = 0;
        /// 切分器交付的 AU 累计数，可与解码输出计数比较处理进度。
        uint64_t aus = 0;
        /// 最近收到的 SR 所报告的当前会话视频发送包数和字节数。
        /// 本地 packets 包含 SR 等数据报，且 SR 报告存在时间滞后，不能直接相减计算丢包。
        uint64_t dev_sent_packets = 0;
        uint64_t dev_sent_octets = 0;
        /// 已匹配会话来源的设备 RTCP SR 被跳过的包数。
        uint64_t other_payload = 0;
        /// 当前来源的 SR 和视频 RTP；video_packets + sr_packets == packets。
        /// 外来源、无法解析的头、其他 PT/RTCP 不参与这三项统计及静默保活。
        uint64_t sr_packets = 0;
        uint64_t video_packets = 0;
        /// RR 发送成功和失败次数。成功只表示交给本地传输层，不证明设备已经收到
        /// 或接受；须结合设备状态和接收证据排查会话过期。
        uint64_t rtcp_sent = 0;
        uint64_t rtcp_failed = 0;
        /// PLI 发送成功次数，配合关键帧、等待丢弃及输出计数观察恢复。
        /// 单独的发送计数不能证明设备响应。
        uint64_t pli_sent = 0;
        /// 丢包后等待 IDR 超时触发的会话重建次数。stall_probe 可隔离这条恢复路径。
        uint64_t stall_restarts = 0;
        /// 被测试选项禁止发送的 PLI 数，用于确认请求分支确实执行。
        uint64_t pli_suppressed = 0;
        /// 探针主动丢包的单调时钟时刻（毫秒），用于测量后备重建延迟。
        uint64_t debug_dropped_at_ms = 0;
        /// 探针注入的解码失败累计数。探针在计数增长时保存快照，按失败之后的增量
        /// 判断恢复行为，避免混入失败前的正常等待。
        uint64_t forced_decode_failures = 0;
        /// 当前会话开始时本地 packets 的快照，用于扣除旧会话累计值。
        /// 该计数仍包含 SR，不能直接与设备视频发送计数相减。
        uint64_t session_packets_base = 0;
        /// 拆包、解码和发布累计耗时，单位毫秒；与调用次数共同用于分析每帧开销。
        double ms_depacketize = 0, ms_decode = 0, ms_publish = 0;
        uint64_t decode_calls = 0;
    };

    /// 在 Device 连接上启动媒体泵；失败时 err 包含原因。
    static std::unique_ptr<FramePump>
        start(maa::ios::remote::Device& device, const Options& options, std::string& err, bool verbose = false);

    ~FramePump();

    FramePump(const FramePump&) = delete;
    FramePump& operator=(const FramePump&) = delete;

    /// 取最新一帧（可能是上一帧的重复副本）。timeout_ms 内一帧都没有则返回 false。
    bool latest(Frame& out, int timeout_ms);

    /// 等待真实解码像素帧。停止、不可用或超时返回 false。
    bool wait_ready(int timeout_ms);
    /// 复制导致 worker 永久终止的错误。
    /// 可恢复的视频不可用标记不在此接口中；不会借用 worker 的字符串。
    [[nodiscard]] std::string capture_error() const;

    /// 通知媒体泵用户刚操作或请求新画面。流仍有新数据时不处理；静默超过可疑
    /// 阈值时查询设备，超过直接重建阈值时重建会话。当前会话申请 20 秒租期，
    /// 通过周期 RR 保活。
    void wake() { wake_requested_ = true; }

    /// 返回是否正在查询状态或重建会话，直到新会话输出首帧或校验通过首个 IDR。
    /// 调用方可据此区分恢复期间尚无新帧和画面静止。标记在状态 RPC 前置位，
    /// 避免调用方在耗时查询期间把旧帧当作操作后的结果。
    [[nodiscard]] bool reviving() const { return reviving_; }

    /// 多次重建仍无法输出画面时标记视频暂不可用，例如帧超过后端能力或
    /// 关键帧持续无输出。调用方可使用截图；后台低频重试，成功解码后清除标记。
    /// 恢复起流已被接受但答复不可用时也置位，此时停止后台重试，等待设备租期释放。
    [[nodiscard]] bool video_unusable() const { return video_unusable_; }

    /// 等待帧号大于 since，返回帧号；超时为 0。
    /// 需要动作后的新画面时，since 使用调用时的 serial() 快照。若仅使用上次
    /// 消费序号，动作前已经产生但尚未消费的旧帧也会立即满足条件。
    uint64_t newer(Frame& out, uint64_t since, int timeout_ms);

    [[nodiscard]] uint64_t serial() const;
    [[nodiscard]] Stats stats() const;
    /// 协商到的视频 payload type，HID 之外的调试用得上；重建或失败时为 0。
    /// getter 只读取 mutex_ 保护的标量快照，不访问 worker 正在替换的会话。
    [[nodiscard]] uint8_t payload_type() const;
    [[nodiscard]] uint16_t receiver_port() const;
    /// 最近已解码像素帧的编码尺寸；还没有有效尺寸时为 0。
    void size(int& width, int& height) const;

private:
    FramePump(maa::ios::remote::Device& device, Options options, bool verbose);
    void loop();
    /// 停止旧会话并创建新会话。首次启动也使用此入口，但不创建线程；
    /// start() 完成恢复时钟等初始化后再启动 worker，避免无锁字段竞争。
    bool restart(std::string& err);
    maa::ios::remote::Device& device_;
    Options options_;
    bool verbose_ = false;

    std::unique_ptr<StreamSession> session_;
    std::thread worker_;
    bool worker_running_ = false;
    /// 仅由启动线程/worker 访问；已接受但不可用的答复使本泵停止重试及续期。
    bool negotiation_invalid_ = false;
    std::string terminal_error_;

    /// 解析器仅由后台线程访问，每次重建会话时替换。
    std::unique_ptr<AnnexBParser> parser_;

    /// 下列帧、尺寸、会话信息及统计由 mutex_ 保护；原子状态另行声明。
    /// 尺寸保留独立快照，查询宽高不需要复制像素。
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    Frame frame_;
    /// 解码缓冲，发布时与 frame_ 交换以复用容量。
    Frame publishing_;
    uint64_t serial_ = 0;
    bool ready_ = false;
    /// 救流进行中：决定去问设备/重起时置位，新会话第一帧交出来时清掉。
    std::atomic<bool> reviving_ { false };
    bool stopping_ = false;
    int width_ = 0;
    int height_ = 0;
    /// session_ 只由 worker 管理；外部读取这两个快照，避免重建时借用已销毁对象。
    uint8_t payload_type_ = 0;
    uint16_t receiver_port_ = 0;
    Stats stats_;
    /// 最近收到数据报的时刻，用于检测完全无包；不能仅依赖序号缺口。
    uint64_t last_packet_ms_ = 0;
    /// 下次发送 RR 的时刻。RTCPTimeoutInterval 从设备上次接收反馈计时，
    /// 周期 RR 用于保持会话，当前间隔见 kRtcpPeriodMs。
    uint64_t next_rtcp_ms_ = 0;
    /// 最早什么时候可以再发一个 PLI（0 = 立刻可发）。见 FramePump.cpp 的 request_keyframe。
    uint64_t next_pli_ms_ = 0;
    /// 上一次发 PLI 的时刻（等待期间每秒会被刷新，所以**不能**拿它算等待期限）。
    uint64_t last_pli_ms_ = 0;
    /// 当前等待干净 IDR 的起点，成功解码后清零。后备重建使用首次请求时间，
    /// 不能使用每次重发都会更新的 last_pli_ms_。
    uint64_t first_pli_ms_ = 0;
    /// 当前"等关键帧"的成因是不是丢包。只有它才走后备重起；超大 NAL 那种成因有自己的
    /// 上限与降级，不能被通用判据抢走（见 FramePump.cpp 的 stalled 那段）。
    bool awaiting_idr_from_loss_ = false;
    /// 测试选项首次注入失败后禁止 PLI 的状态，仅在工作线程访问。
    bool pli_off_ = false;
    /// 最近成功输出关键帧，或解码模式通过参数/语法/时间校验的 IDR 时刻。
    /// 启动和后备重建前以当前时刻建立等待基线；不完整或校验失败的关键帧
    /// 不更新它，避免持续失败无限延后恢复。
    uint64_t last_decoded_keyframe_ms_ = 0;
    /// 有超大 NAL 被丢、需要重起会话。由 AU 回调置位，收包线程消费。
    std::atomic<bool> oversized_restart_ { false };
    /// 用户刚刚动过（见 wake()）。由收包线程取走并做一次"流还活着吗"的检查。
    std::atomic<bool> wake_requested_ { false };
    uint64_t last_restart_ms_ = 0;
    /// 因后端长度限制连续重建的次数；成功解码后归零，上限后进入低频重试。
    int oversized_restarts_ = 0;
    std::atomic<bool> video_unusable_ { false };
    /// 当前会话是否成功解出关键帧，。
    std::atomic<bool> ever_keyframe_ { false };
    uint64_t session_start_ms_ = 0;
    int nokey_restarts_ = 0;
    /// 丢包后等待完整关键帧，避免继续发布依赖受损参考链的画面。
    std::atomic<bool> need_keyframe_ { false };
    uint64_t loss_seen_ = 0;
};

/// 编码帧里"真正显示出来"的那一块。
struct DisplayCrop
{
    int x = 0, y = 0, w = 0, h = 0;
};

/// 设备查询不可用时的编码尺寸到可见区映射表，文件回放也使用此表。
/// 主路径为 remote::fetch_display_info / displayinfoupdates。
/// iPhone 13 mini 的 1136x2464 编码对应 1125x2436 可见区，右侧 11 px、
/// 底部 28 px 为填充；触摸归一化必须使用可见区尺寸。
/// 未知机型保留完整编码尺寸，避免猜测裁剪。SPS conformance window 解析尚未实现。
[[nodiscard]] DisplayCrop display_crop(int coded_w, int coded_h);

} // namespace maa::ios::media
