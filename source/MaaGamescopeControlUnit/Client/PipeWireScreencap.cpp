#include "PipeWireScreencap.h"

#include <cstring>

#include <opencv2/imgproc.hpp>

#include <spa/debug/types.h>
#include <spa/param/video/format-utils.h>

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

struct SpaFormatInfo
{
    enum spa_video_format format;
    int channels;
    int cv_conversion;
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

static const struct spa_pod* build_format_pod(struct spa_pod_builder* b)
{
    static constexpr struct spa_rectangle kDefRect = { 1280, 720 };
    static constexpr struct spa_rectangle kMinRect = { 1, 1 };
    static constexpr struct spa_rectangle kMaxRect = { 4096, 4096 };
    static constexpr struct spa_fraction kDefFps = { 30, 1 };
    static constexpr struct spa_fraction kMinFps = { 0, 1 };
    static constexpr struct spa_fraction kMaxFps = { 1000, 1 };

    return (const struct spa_pod*)spa_pod_builder_add_object(
        b,
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
}

static const struct spa_pod* build_buffer_pod(struct spa_pod_builder* b)
{
    int32_t data_types = (1 << SPA_DATA_MemFd) | (1 << SPA_DATA_MemPtr);
    return (const struct spa_pod*)spa_pod_builder_add_object(
        b,
        SPA_TYPE_OBJECT_ParamBuffers,
        SPA_PARAM_Buffers,
        SPA_PARAM_BUFFERS_dataType,
        SPA_POD_Int(data_types),
        0);
}

PipeWireScreencap::PipeWireScreencap(uint32_t node_id)
    : node_id_(node_id)
{
    LogFunc << VAR(node_id_);
}

PipeWireScreencap::~PipeWireScreencap()
{
    LogFunc;
    shutdown();
}

bool PipeWireScreencap::init()
{
    LogFunc;

    pw_init(nullptr, nullptr);

    tloop_ = pw_thread_loop_new("maa-gs-screencap", nullptr);
    if (!tloop_) {
        LogError << "pw_thread_loop_new failed";
        return false;
    }

    struct pw_loop* loop = pw_thread_loop_get_loop(tloop_);

    uint8_t pod_buf[4096];
    spa_pod_builder b = SPA_POD_BUILDER_INIT(pod_buf, sizeof(pod_buf));
    const struct spa_pod* fmt_pod = build_format_pod(&b);
    const struct spa_pod* buf_pod = build_buffer_pod(&b);

    const struct spa_pod* stream_params[] = { fmt_pod, buf_pod };
    struct pw_properties* stream_props =
        pw_properties_new(PW_KEY_MEDIA_TYPE, "Video", PW_KEY_MEDIA_CATEGORY, "Capture", PW_KEY_MEDIA_ROLE, "Screen", nullptr);

    stream_ = pw_stream_new_simple(loop, "maa-gs-screencap", stream_props, &stream_events_, this);
    if (!stream_) {
        LogError << "pw_stream_new_simple failed";
        return false;
    }

    int rc = pw_stream_connect(
        stream_,
        PW_DIRECTION_INPUT,
        node_id_,
        static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS),
        stream_params,
        2);
    if (rc < 0) {
        LogError << "pw_stream_connect failed" << VAR(rc);
        return false;
    }

    pw_thread_loop_lock(tloop_);

    if (pw_thread_loop_start(tloop_) < 0) {
        pw_thread_loop_unlock(tloop_);
        LogError << "pw_thread_loop_start failed";
        return false;
    }

    pw_thread_loop_timed_wait(tloop_, 3);
    if (width_ == 0) {
        pw_thread_loop_unlock(tloop_);
        LogError << "format negotiation timed out";
        return false;
    }

    pw_thread_loop_unlock(tloop_);

    connected_ = true;
    LogInfo << "PipeWire screencap ready" << VAR(width_) << VAR(height_);
    return true;
}

bool PipeWireScreencap::screencap(cv::Mat& image, std::chrono::milliseconds timeout)
{
    if (!tloop_) {
        LogError << "not initialized";
        return false;
    }

    pw_thread_loop_lock(tloop_);

    capture_requested_ = true;

    int timeout_sec = static_cast<int>((timeout.count() + 999) / 1000);
    pw_thread_loop_timed_wait(tloop_, timeout_sec);

    if (capture_requested_) {
        capture_requested_ = false;
        pw_thread_loop_unlock(tloop_);
        LogWarn << "screencap timeout";
        return false;
    }

    cv::Mat frame = std::move(latest_frame_);
    pw_thread_loop_unlock(tloop_);

    const SpaFormatInfo* fmt_info = spa_format_info(video_format_);
    if (!fmt_info) {
        LogError << "unsupported video format" << VAR(video_format_);
        return false;
    }

    if (fmt_info->cv_conversion < 0) {
        image = std::move(frame);
    }
    else {
        cv::cvtColor(frame, image, fmt_info->cv_conversion);
    }
    return true;
}

void PipeWireScreencap::shutdown()
{
    if (!tloop_) {
        return;
    }

    pw_thread_loop_lock(tloop_);
    pw_thread_loop_signal(tloop_, false);
    pw_thread_loop_unlock(tloop_);

    pw_thread_loop_stop(tloop_);

    if (stream_) {
        pw_stream_destroy(stream_);
        stream_ = nullptr;
    }

    pw_thread_loop_destroy(tloop_);
    tloop_ = nullptr;

    pw_deinit();
    LogInfo << "PipeWire shutdown complete";
}

void PipeWireScreencap::on_param_changed(void* data, uint32_t id, const struct spa_pod* param)
{
    if (id != SPA_PARAM_Format || param == nullptr) {
        return;
    }

    auto* self = static_cast<PipeWireScreencap*>(data);

    struct spa_video_info info = {};
    if (spa_format_video_raw_parse(param, &info.info.raw) < 0) {
        LogError << "spa_format_video_raw_parse failed";
        return;
    }

    const SpaFormatInfo* fmt_info = spa_format_info(info.info.raw.format);
    if (!fmt_info) {
        LogError << "Unsupported video format" << VAR(info.info.raw.format);
        return;
    }

    self->width_ = info.info.raw.size.width;
    self->height_ = info.info.raw.size.height;
    self->video_format_ = info.info.raw.format;
    self->channels_ = fmt_info->channels;

    pw_thread_loop_signal(self->tloop_, false);

    LogInfo << "format negotiated" << VAR(self->width_) << VAR(self->height_)
            << VAR(spa_debug_type_find_name(spa_type_video_format, self->video_format_));
}

void PipeWireScreencap::on_process(void* data)
{
    auto* self = static_cast<PipeWireScreencap*>(data);

    struct pw_buffer* latest = nullptr;
    while (struct pw_buffer* b = pw_stream_dequeue_buffer(self->stream_)) {
        if (latest) {
            pw_stream_queue_buffer(self->stream_, latest);
        }
        latest = b;
    }

    if (!latest) {
        return;
    }

    if (!self->capture_requested_) {
        pw_stream_queue_buffer(self->stream_, latest);
        return;
    }

    struct spa_buffer* spa_buf = latest->buffer;
    if (!spa_buf || spa_buf->n_datas < 1) {
        pw_stream_queue_buffer(self->stream_, latest);
        return;
    }

    struct spa_data* plane = &spa_buf->datas[0];
    if (!plane->data || plane->chunk->size == 0) {
        LogWarn << "PipeWire screencap: empty buffer data";
        pw_stream_queue_buffer(self->stream_, latest);
        return;
    }

    struct spa_meta_header* header =
        static_cast<struct spa_meta_header*>(spa_buffer_find_meta_data(spa_buf, SPA_META_Header, sizeof(*header)));
    if (header && (header->flags & SPA_META_HEADER_FLAG_CORRUPTED)) {
        LogWarn << "PipeWire screencap: corrupted frame";
        pw_stream_queue_buffer(self->stream_, latest);
        return;
    }

    int row_bytes = self->width_ * self->channels_;
    int stride = plane->chunk->stride > 0 ? static_cast<int>(plane->chunk->stride) : row_bytes;
    self->latest_frame_.create(self->height_, self->width_, CV_8UC(self->channels_));

    auto* src = static_cast<uint8_t*>(plane->data);
    if (stride == row_bytes) {
        std::memcpy(self->latest_frame_.data, src, self->height_ * stride);
    }
    else {
        for (int row = 0; row < self->height_; ++row) {
            std::memcpy(self->latest_frame_.ptr(row), src + row * stride, row_bytes);
        }
    }

    self->capture_requested_ = false;
    pw_stream_queue_buffer(self->stream_, latest);
    pw_thread_loop_signal(self->tloop_, false);
}

MAA_CTRL_UNIT_NS_END
