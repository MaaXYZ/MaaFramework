#include "PipeWireScreencap.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <string>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <linux/dma-buf.h>

#include <pipewire/pipewire.h>
#include <pipewire/stream.h>
#include <pipewire/thread-loop.h>
#include <spa/param/buffers.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/video/raw.h>
#include <spa/pod/builder.h>
#include <spa/pod/iter.h>

#include <chrono>

#include <opencv2/imgproc.hpp>

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

static constexpr int kPWFirstFrameTimeoutSec = 3;

// 需持 loop 锁调用; 唤醒可能不来自目标事件, 谓词不成立就继续等
template <typename Pred>
static bool loop_wait_until(pw_thread_loop* loop, int timeout_sec, Pred pred)
{
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout_sec);
    while (!pred()) {
        auto remain = deadline - std::chrono::steady_clock::now();
        if (remain <= std::chrono::seconds(0)) {
            return pred();
        }
        int remain_sec = std::max(1, static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(remain).count()));
        if (pw_thread_loop_timed_wait(loop, remain_sec) < 0) {
            return pred();
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// SPA format -> OpenCV conversion table
// ---------------------------------------------------------------------------

struct SpaFormatInfo
{
    enum spa_video_format format;
    int channels;
    int cv_conversion; // -1 表示无需转换 (本身即为 BGR)
};

static constexpr SpaFormatInfo kSpaFormatTable[] = {
    { SPA_VIDEO_FORMAT_BGRA, 4, cv::COLOR_BGRA2BGR },     { SPA_VIDEO_FORMAT_BGRx, 4, cv::COLOR_BGRA2BGR },
    { SPA_VIDEO_FORMAT_RGBA, 4, cv::COLOR_RGBA2BGR },     { SPA_VIDEO_FORMAT_RGBx, 4, cv::COLOR_RGBA2BGR },
    { SPA_VIDEO_FORMAT_RGB, 3, cv::COLOR_RGB2BGR },       { SPA_VIDEO_FORMAT_BGR, 3, -1 },
    { SPA_VIDEO_FORMAT_YUY2, 2, cv::COLOR_YUV2BGR_YUY2 },
};

static const SpaFormatInfo* spa_format_info(enum spa_video_format format)
{
    for (const auto& info : kSpaFormatTable) {
        if (info.format == format) {
            return &info;
        }
    }
    return nullptr;
}

// CPU 读 dma-buf 映射前后必须用 DMA_BUF_IOCTL_SYNC 成对打括号:
// 驱动对 buffer 可能给缓存映射, 不显式同步会读到设备未写完的残帧 (实测出现过)
static bool dma_buf_sync_access(int fd, bool start)
{
    struct dma_buf_sync sync = {
        .flags = static_cast<__u64>(DMA_BUF_SYNC_READ | (start ? DMA_BUF_SYNC_START : DMA_BUF_SYNC_END)),
    };
    if (ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync) < 0) {
        static bool warned = false;
        if (!warned) {
            warned = true;
            LogWarn << "DMA_BUF_IOCTL_SYNC failed: " << strerror(errno);
        }
        return false;
    }
    return true;
}

PipeWireScreencap::PipeWireScreencap(int pipewire_fd, uint32_t pipewire_node_id)
    : pipewire_fd_(pipewire_fd)
    , pipewire_node_id_(pipewire_node_id)
{
    // PipeWire global init is safe to call multiple times (uses an internal refcount).
    ::pw_init(nullptr, nullptr);
}

PipeWireScreencap::~PipeWireScreencap()
{
    close();
}

void PipeWireScreencap::close_internal()
{
    // Tear down PipeWire objects in reverse order of creation.
    // This is safe even if some pointers are null (checks before use).

    if (pw_thread_loop_) {
        pw_thread_loop_stop(pw_thread_loop_);
    }

    if (pw_stream_) {
        spa_hook_remove(&stream_hook_);
        pw_stream_destroy(pw_stream_);
        pw_stream_ = nullptr;
    }

    if (pw_core_) {
        spa_hook_remove(&core_hook_);
        pw_core_disconnect(pw_core_);
        pw_core_ = nullptr;
    }

    if (pw_context_) {
        pw_context_destroy(pw_context_);
        pw_context_ = nullptr;
    }

    if (pw_thread_loop_) {
        pw_thread_loop_destroy(pw_thread_loop_);
        pw_thread_loop_ = nullptr;
    }

    // Reset all state
    connected_ = false;
    stream_active_ = false;
    frame_wanted_ = false;
    frame_width_ = 0;
    frame_height_ = 0;
    frame_format_ = SPA_VIDEO_FORMAT_UNKNOWN;
    latest_frame_ = cv::Mat();
}

bool PipeWireScreencap::init()
{
    if (connected_) {
        return true;
    }

    // Clean up any stale state from a previous failed session
    close_internal();

    if (!pw_init() || !pw_create_stream() || !pw_connect_stream(pipewire_node_id_)) {
        close_internal();
        return false;
    }

    // 协商是异步的, 节点不存在等失败在此超时, 与首帧失败分开诊断
    static constexpr int kNegotiationTimeoutSec = 3;
    pw_thread_loop_lock(pw_thread_loop_);
    if (!loop_wait_until(pw_thread_loop_, kNegotiationTimeoutSec, [this]() { return frame_width_ > 0 && frame_height_ > 0; })) {
        pw_thread_loop_unlock(pw_thread_loop_);
        LogError << "PipeWire format negotiation timed out";
        close_internal();
        return false;
    }

    // 激活必须晚于 buffers reply, 否则 producer 不送帧
    pw_stream_set_active(pw_stream_, true);
    stream_active_ = true;

    // 首帧验证 producer 真的在送帧
    frame_wanted_ = true;
    bool got_frame = loop_wait_until(pw_thread_loop_, kPWFirstFrameTimeoutSec, [this]() { return !latest_frame_.empty(); });
    frame_wanted_ = false;
    if (!got_frame) {
        pw_thread_loop_unlock(pw_thread_loop_);
        LogError << "Timeout waiting for first PipeWire frame";
        close_internal();
        return false;
    }

    // 验证帧先于任何请求, 交付它会让首次 screencap 拿到陈旧画面
    latest_frame_ = cv::Mat();
    pw_thread_loop_unlock(pw_thread_loop_);

    connected_ = true;
    LogInfo << "PipeWire screencap ready" << VAR(frame_width_) << VAR(frame_height_);
    return true;
}

void PipeWireScreencap::close()
{
    close_internal();
}

bool PipeWireScreencap::connected() const
{
    return connected_;
}

std::optional<cv::Mat> PipeWireScreencap::screencap()
{
    if (!connected_) {
        return std::nullopt;
    }

    pw_thread_loop_lock(pw_thread_loop_);

    if (!stream_active_) {
        // inactive() 已停流, 重新激活后才会有新帧
        pw_stream_set_active(pw_stream_, true);
        stream_active_ = true;
    }

    if (latest_frame_.empty()) {
        // 置位后回调才会拷贝, 否则永远等不到帧
        frame_wanted_ = true;
        bool got_frame = loop_wait_until(pw_thread_loop_, kPWFirstFrameTimeoutSec, [this]() { return !latest_frame_.empty(); });
        frame_wanted_ = false;
        if (!got_frame) {
            pw_thread_loop_unlock(pw_thread_loop_);
            LogError << "Timeout waiting for PipeWire frame";
            return std::nullopt;
        }
    }

    // 转换耗时, 留在锁外以免停摆 loop 线程
    cv::Mat raw = std::move(latest_frame_);
    enum spa_video_format format = frame_format_;
    pw_thread_loop_unlock(pw_thread_loop_);

    const SpaFormatInfo* fmt_info = spa_format_info(format);
    if (!fmt_info) {
        LogError << "Unsupported video format" << VAR(static_cast<int>(format));
        return std::nullopt;
    }

    cv::Mat image;
    if (fmt_info->cv_conversion < 0) {
        image = std::move(raw);
    }
    else {
        cv::cvtColor(raw, image, fmt_info->cv_conversion);
    }
    return image;
}

void PipeWireScreencap::inactive()
{
    ScreencapBase::inactive();
    if (!pw_stream_ || !pw_thread_loop_ || !stream_active_) {
        return;
    }

    pw_thread_loop_lock(pw_thread_loop_);
    pw_stream_set_active(pw_stream_, false);
    stream_active_ = false;
    pw_thread_loop_unlock(pw_thread_loop_);
}

// ===========================================================================
// PipeWire implementation
// ===========================================================================

bool PipeWireScreencap::pw_init()
{
    // Create pw_thread_loop (standard event loop with integrated bg thread)
    pw_thread_loop_ = pw_thread_loop_new("MaaScreencap", nullptr);
    if (!pw_thread_loop_) {
        LogError << "Failed to create PipeWire thread loop";
        return false;
    }

    struct pw_loop* loop = pw_thread_loop_get_loop(pw_thread_loop_);

    pw_context_ = pw_context_new(loop, nullptr, 0);
    if (!pw_context_) {
        LogError << "Failed to create PipeWire context";
        close_internal();
        return false;
    }

    if (pipewire_fd_ >= 0) {
        // 通过 portal 提供的 fd 连接远端 PipeWire 实例 (KDE/KWin 路径)
        pw_core_ = pw_context_connect_fd(pw_context_, pipewire_fd_, nullptr, 0);
    }
    else {
        // 连接会话 PipeWire daemon (窗口捕获路径)
        pw_core_ = pw_context_connect(pw_context_, nullptr, 0);
    }
    if (!pw_core_) {
        LogError << "Failed to connect to PipeWire";
        close_internal();
        return false;
    }

    // pw_context_connect_fd takes ownership of pipewire_fd_
    pipewire_fd_ = -1;

    // Register core event listener for error detection (e.g. EPIPE on disconnect).
    static const struct pw_core_events core_events = {
        .version = PW_VERSION_CORE_EVENTS,
        .error = pw_on_core_error,
    };
    pw_core_add_listener(pw_core_, &core_hook_, &core_events, this);

    return true;
}

bool PipeWireScreencap::pw_create_stream()
{
    struct pw_properties* props =
        pw_properties_new(PW_KEY_MEDIA_TYPE, "Video", PW_KEY_MEDIA_CATEGORY, "Capture", PW_KEY_MEDIA_ROLE, "Screen", nullptr);

    // Use pw_stream_new (not pw_stream_new_simple) to avoid exception
    // unwinding ABI incompatibility between clang/libc++ and PipeWire's glibc.
    pw_stream_ = pw_stream_new(pw_core_, "MaaFramework Screencap", props);
    if (!pw_stream_) {
        LogError << "Failed to create PipeWire stream: " << strerror(errno);
        if (props) {
            pw_properties_free(props);
        }
        return false;
    }

    // Set up event listeners
    static const struct pw_stream_events stream_events = {
        .version = PW_VERSION_STREAM_EVENTS,
        .state_changed = pw_on_stream_state_changed,
        .param_changed = pw_on_stream_param_changed,
        .process = pw_on_stream_process,
    };
    spa_zero(stream_hook_);
    pw_stream_add_listener(pw_stream_, &stream_hook_, &stream_events, this);

    return true;
}

bool PipeWireScreencap::pw_connect_stream(uint32_t node_id)
{
    // 接受任意分辨率/帧率; 多声明几种格式: producer 可能 fixate 非 BGRA 格式, 均可按 kSpaFormatTable 转换。
    uint8_t buffer[4096];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    static const struct spa_rectangle kDefRect = { 1280, 720 };
    static const struct spa_rectangle kMinRect = { 1, 1 };
    static const struct spa_rectangle kMaxRect = { 4096, 4096 };
    static const struct spa_fraction kDefFps = { 30, 1 };
    static const struct spa_fraction kMinFps = { 0, 1 };
    static const struct spa_fraction kMaxFps = { 1000, 1 };

    // 官方 DMA-BUF 协商文档要求消费者同时声明两个 EnumFormat:
    // 带 modifier 的 (dmabuf, gamescope 靠 modifier 有无决定 DmaBuf/MemFd) 与不带的 (shm fallback, portal/摄像头等不支持 modifier 的 producer 只会命中此 offer)。
    const struct spa_pod* fmt_dmabuf = (const struct spa_pod*)spa_pod_builder_add_object(
        &b,
        SPA_TYPE_OBJECT_Format,
        SPA_PARAM_EnumFormat,
        SPA_FORMAT_mediaType,
        SPA_POD_Id(SPA_MEDIA_TYPE_video),
        SPA_FORMAT_mediaSubtype,
        SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
        SPA_FORMAT_VIDEO_format,
        SPA_POD_CHOICE_ENUM_Id(
            7,
            SPA_VIDEO_FORMAT_RGB,
            SPA_VIDEO_FORMAT_RGBA,
            SPA_VIDEO_FORMAT_RGBx,
            SPA_VIDEO_FORMAT_BGR,
            SPA_VIDEO_FORMAT_BGRA,
            SPA_VIDEO_FORMAT_BGRx,
            SPA_VIDEO_FORMAT_YUY2),
        SPA_FORMAT_VIDEO_size,
        SPA_POD_CHOICE_RANGE_Rectangle(&kDefRect, &kMinRect, &kMaxRect),
        SPA_FORMAT_VIDEO_framerate,
        SPA_POD_CHOICE_RANGE_Fraction(&kDefFps, &kMinFps, &kMaxFps),
        SPA_FORMAT_VIDEO_modifier,
        SPA_POD_Long(0), // Linear: dmabuf 才可被 CPU mmap 读取
        0);

    const struct spa_pod* fmt_shm = (const struct spa_pod*)spa_pod_builder_add_object(
        &b,
        SPA_TYPE_OBJECT_Format,
        SPA_PARAM_EnumFormat,
        SPA_FORMAT_mediaType,
        SPA_POD_Id(SPA_MEDIA_TYPE_video),
        SPA_FORMAT_mediaSubtype,
        SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
        SPA_FORMAT_VIDEO_format,
        SPA_POD_CHOICE_ENUM_Id(
            7,
            SPA_VIDEO_FORMAT_RGB,
            SPA_VIDEO_FORMAT_RGBA,
            SPA_VIDEO_FORMAT_RGBx,
            SPA_VIDEO_FORMAT_BGR,
            SPA_VIDEO_FORMAT_BGRA,
            SPA_VIDEO_FORMAT_BGRx,
            SPA_VIDEO_FORMAT_YUY2),
        SPA_FORMAT_VIDEO_size,
        SPA_POD_CHOICE_RANGE_Rectangle(&kDefRect, &kMinRect, &kMaxRect),
        SPA_FORMAT_VIDEO_framerate,
        SPA_POD_CHOICE_RANGE_Fraction(&kDefFps, &kMinFps, &kMaxFps),
        0);

    int32_t data_types = (1 << SPA_DATA_MemFd) | (1 << SPA_DATA_DmaBuf);
    const struct spa_pod* buf_pod = (const struct spa_pod*)spa_pod_builder_add_object(
        &b,
        SPA_TYPE_OBJECT_ParamBuffers,
        SPA_PARAM_Buffers,
        SPA_PARAM_BUFFERS_dataType,
        SPA_POD_CHOICE_FLAGS_Int(data_types),
        0);

    const struct spa_pod* params[3] = { fmt_dmabuf, fmt_shm, buf_pod };

    constexpr auto stream_flags =
        static_cast<enum pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_DONT_RECONNECT | PW_STREAM_FLAG_MAP_BUFFERS);

    // Start the bg thread BEFORE pw_stream_connect
    if (pw_thread_loop_start(pw_thread_loop_) < 0) {
        LogError << "Failed to start pw_thread_loop";
        return false;
    }

    pw_thread_loop_lock(pw_thread_loop_);
    int ret = pw_stream_connect(pw_stream_, PW_DIRECTION_INPUT, node_id, stream_flags, params, 3);
    pw_thread_loop_unlock(pw_thread_loop_);

    if (ret < 0) {
        LogError << "Failed to connect PipeWire stream to node" << node_id;
        return false;
    }

    return true;
}

// ===========================================================================
// PipeWire callbacks
// ===========================================================================

void PipeWireScreencap::pw_on_core_error(void* data, uint32_t id, int seq, int res, const char* message)
{
    auto* self = static_cast<PipeWireScreencap*>(data);
    LogError << "PipeWire core error: id=" << id << " seq=" << seq << " res=" << res << " msg=" << (message ? message : "");

    if (id == PW_ID_CORE && res == -EPIPE) {
        self->connected_ = false;
    }
}

void PipeWireScreencap::pw_on_stream_state_changed(
    void* data,
    enum pw_stream_state old_state,
    enum pw_stream_state new_state,
    const char* error)
{
    auto* self = static_cast<PipeWireScreencap*>(data);
    (void)self;
    (void)old_state;

    if (new_state < 0) {
        LogError << "Stream error: " << (error ? error : "(unknown)");
    }
    else if (new_state == PW_STREAM_STATE_STREAMING && error) {
        LogWarn << "Stream error: " << error;
    }
}

void PipeWireScreencap::pw_on_stream_param_changed(void* data, uint32_t id, const struct spa_pod* param)
{
    auto* self = static_cast<PipeWireScreencap*>(data);

    if (!param || id != SPA_PARAM_Format) {
        return;
    }

    // gamescope 的 Format pod 以 Choice(None) 包装定值, spa_format_video_raw_parse 对此会解析失败, 需手动取 choice 默认值。
    uint32_t width = 0;
    uint32_t height = 0;
    enum spa_video_format format = SPA_VIDEO_FORMAT_UNKNOWN;

    struct spa_video_info info;
    if (spa_format_parse(param, &info.media_type, &info.media_subtype) == 0 && info.media_type == SPA_MEDIA_TYPE_video
        && info.media_subtype == SPA_MEDIA_SUBTYPE_raw && spa_format_video_raw_parse(param, &info.info.raw) == 0) {
        width = info.info.raw.size.width;
        height = info.info.raw.size.height;
        format = static_cast<enum spa_video_format>(info.info.raw.format);
    }

    if (width == 0 || height == 0 || format == SPA_VIDEO_FORMAT_UNKNOWN) {
        const struct spa_pod_prop* prop;
        SPA_POD_OBJECT_FOREACH((struct spa_pod_object*)param, prop)
        {
            const struct spa_pod* val = &prop->value;
            if (prop->key == SPA_FORMAT_VIDEO_size) {
                if (spa_pod_is_choice(val)) {
                    // choice 的第一个备选值即默认值
                    if (SPA_POD_CHOICE_VALUE_TYPE(val) == SPA_TYPE_Rectangle) {
                        const struct spa_rectangle* rect = (const struct spa_rectangle*)SPA_POD_CHOICE_VALUES(val);
                        if (rect->width > 0 && rect->height > 0) {
                            width = rect->width;
                            height = rect->height;
                        }
                    }
                }
                else if (spa_pod_is_rectangle(val)) {
                    struct spa_rectangle* rect = static_cast<struct spa_rectangle*>(SPA_POD_BODY(val));
                    if (rect->width > 0 && rect->height > 0) {
                        width = rect->width;
                        height = rect->height;
                    }
                }
            }
            else if (prop->key == SPA_FORMAT_VIDEO_format) {
                if (spa_pod_is_choice(val)) {
                    if (SPA_POD_CHOICE_VALUE_TYPE(val) == SPA_TYPE_Id) {
                        format = static_cast<enum spa_video_format>(*(uint32_t*)SPA_POD_CHOICE_VALUES(val));
                    }
                }
                else if (spa_pod_is_id(val)) {
                    format = static_cast<enum spa_video_format>(*(uint32_t*)SPA_POD_BODY(val));
                }
            }
        }
    }

    const SpaFormatInfo* fmt_info = spa_format_info(format);
    if (!fmt_info) {
        LogError << "Unsupported negotiated video format" << VAR(static_cast<int>(format));
        return;
    }
    if (width == 0 || height == 0) {
        LogError << "Format negotiation did not include a valid frame size";
        return;
    }

    self->frame_width_ = static_cast<int>(width);
    self->frame_height_ = static_cast<int>(height);
    self->frame_format_ = format;

    // init() 在 loop 锁内等待协商结果
    pw_thread_loop_signal(self->pw_thread_loop_, false);
}

void PipeWireScreencap::pw_on_stream_process(void* data)
{
    auto* self = static_cast<PipeWireScreencap*>(data);

    // 排空只留最新: requeue 是零拷贝链表操作, 保证交付请求之后的最新画面
    struct pw_buffer* latest = nullptr;
    while (struct pw_buffer* buf = pw_stream_dequeue_buffer(self->pw_stream_)) {
        if (latest) {
            pw_stream_queue_buffer(self->pw_stream_, latest);
        }
        latest = buf;
    }

    if (!latest) {
        return;
    }

    struct spa_buffer* spa_buf = latest->buffer;
    if (!spa_buf || spa_buf->n_datas < 1) {
        pw_stream_queue_buffer(self->pw_stream_, latest);
        return;
    }

    // 空闲时逐帧拷贝会占满 CPU
    if (!self->frame_wanted_) {
        pw_stream_queue_buffer(self->pw_stream_, latest);
        return;
    }

    if (self->copy_raw_frame(spa_buf)) {
        self->frame_wanted_ = false;
        pw_thread_loop_signal(self->pw_thread_loop_, false);
    }

    pw_stream_queue_buffer(self->pw_stream_, latest);
}

// ===========================================================================
// Frame processing
// ===========================================================================

bool PipeWireScreencap::copy_raw_frame(const struct spa_buffer* spa_buf)
{
    const SpaFormatInfo* fmt_info = spa_format_info(frame_format_);
    if (!fmt_info || frame_width_ <= 0 || frame_height_ <= 0) {
        return false;
    }

    const struct spa_data& data = spa_buf->datas[0];

    // Obtain a CPU-accessible pointer to the buffer data
    void* mapped_ptr = data.data;
    bool need_unmap = false;
    bool need_sync_end = false;
    size_t map_size = 0;

    if (!mapped_ptr) {
        if (data.type == SPA_DATA_MemFd && data.fd >= 0) {
            map_size = data.maxsize > 0 ? data.maxsize : data.chunk ? data.chunk->size : 0;
            if (map_size > 0) {
                mapped_ptr = mmap(nullptr, map_size, PROT_READ, MAP_PRIVATE, data.fd, 0);
                if (mapped_ptr == MAP_FAILED) {
                    LogError << "mmap of PipeWire buffer failed: " << strerror(errno);
                    return false;
                }
                need_unmap = true;
            }
        }
        else if (data.type == SPA_DATA_DmaBuf && data.fd >= 0) {
            // Linear DMA-BUF (modifier 0) can be CPU-mapped directly.
            // GBM 可能给 stride 加 padding, 映射不足 stride*height 会越界触发 SIGBUS。
            uint32_t stride = data.chunk ? data.chunk->stride : 0;
            if (stride == 0) {
                stride = static_cast<uint32_t>(frame_width_) * fmt_info->channels;
            }
            map_size = static_cast<size_t>(stride) * frame_height_;
            mapped_ptr = mmap(nullptr, map_size, PROT_READ, MAP_SHARED, data.fd, 0);
            if (mapped_ptr == MAP_FAILED) {
                LogError << "mmap of DMA-BUF buffer failed: " << strerror(errno);
                return false;
            }
            need_unmap = true;
            need_sync_end = dma_buf_sync_access(data.fd, true);
        }
        else {
            LogError << "Unsupported buffer type: " << data.type;
            return false;
        }
    }

    if (!mapped_ptr) {
        return false;
    }

    // 统一收尾: 读完后先 END 同步再 munmap, 保证 ioctl 括号成对
    auto unmap = [&]() {
        if (need_unmap) {
            if (need_sync_end) {
                dma_buf_sync_access(data.fd, false);
            }
            munmap(mapped_ptr, map_size);
        }
    };

    // GBM 的 stride padding 使行间不连续
    int row_bytes = frame_width_ * fmt_info->channels;
    uint32_t stride = data.chunk ? data.chunk->stride : 0;
    if (stride == 0) {
        stride = static_cast<uint32_t>(row_bytes);
    }

    latest_frame_.create(frame_height_, frame_width_, CV_8UC(fmt_info->channels));
    if (stride == static_cast<uint32_t>(row_bytes)) {
        std::memcpy(latest_frame_.data, mapped_ptr, static_cast<size_t>(frame_height_) * stride);
    }
    else {
        for (int row = 0; row < frame_height_; ++row) {
            std::memcpy(latest_frame_.ptr(row), static_cast<uint8_t*>(mapped_ptr) + static_cast<size_t>(row) * stride, row_bytes);
        }
    }

    unmap();

    return true;
}

MAA_CTRL_UNIT_NS_END
