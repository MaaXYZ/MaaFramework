#include "PipeWireScreencap.h"

#include <cerrno>
#include <string>

#include <sys/mman.h>
#include <unistd.h>

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

// ---------------------------------------------------------------------------
// PipeWire buffer negotiation constants
// ---------------------------------------------------------------------------
/// Number of buffers to request (min / default / max) for SPA_PARAM_BUFFERS_buffers.
static constexpr int32_t kPWBufferCountMin = 2;
static constexpr int32_t kPWBufferCountDefault = 4;
static constexpr int32_t kPWBufferCountMax = 8;

/// Number of memory blocks per buffer.
static constexpr int32_t kPWBufferBlocks = 1;

/// Buffer alignment (bytes).
static constexpr int32_t kPWBufferAlign = 16;

/// Bytes per pixel for BGRA format.
static constexpr int kBytesPerPixel = 4;

PipeWireScreencap::PipeWireScreencap(int pipewire_fd, uint32_t pipewire_node_id, int screen_width, int screen_height)
    : pipewire_fd_(pipewire_fd)
    , pipewire_node_id_(pipewire_node_id)
    , screen_width_(screen_width)
    , screen_height_(screen_height)
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
    pw_loop_ = nullptr;

    delete stream_events_;
    stream_events_ = nullptr;

    // Reset all state
    connected_ = false;
    open_attempted_ = false;
    frame_available_ = false;
    dmabuf_warned_ = false;
    frame_width_ = 0;
    frame_height_ = 0;

    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        latest_frame_ = cv::Mat();
    }
}

bool PipeWireScreencap::open()
{
    if (connected_) {
        return true;
    }

    // Clean up any stale state from a previous failed session
    close_internal();

    // Initialize PipeWire (connect core, create stream, connect)
    if (!pw_init()) {
        close_internal();
        return false;
    }

    if (!pw_create_stream()) {
        close_internal();
        return false;
    }

    if (!pw_connect_stream(pipewire_node_id_)) {
        close_internal();
        return false;
    }

    connected_ = true;
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
    // Lazy init: try once on first screencap() call
    if (!connected_ && !open_attempted_) {
        open_attempted_ = true;
        if (!open()) {
            return std::nullopt;
        }
    }

    if (!connected_) {
        return std::nullopt;
    }

    // Wait for the first frame with a timeout.
    // Subsequent calls return the cached frame immediately without waiting.
    std::unique_lock<std::mutex> lock(frame_mutex_);
    if (latest_frame_.empty()) {
        if (!frame_cv_.wait_for(lock, std::chrono::seconds(2), [this]() { return !latest_frame_.empty(); })) {
            LogError << "Timeout waiting for first PipeWire frame";
            return std::nullopt;
        }
    }

    cv::Mat target;
    latest_frame_.copyTo(target);
    std::optional ret(target);
    return ret;
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

    pw_loop_ = pw_thread_loop_get_loop(pw_thread_loop_);

    pw_context_ = pw_context_new(pw_loop_, nullptr, 0);
    if (!pw_context_) {
        LogError << "Failed to create PipeWire context";
        close_internal();
        return false;
    }

    if (pipewire_fd_ < 0) {
        LogError << "Invalid PipeWire FD (missing OpenPipeWireRemote?)";
        close_internal();
        return false;
    }

    // Connect to KWin's private PipeWire instance via the portal FD
    pw_core_ = pw_context_connect_fd(pw_context_, pipewire_fd_, nullptr, 0);
    if (!pw_core_) {
        LogError << "Failed to connect via portal PipeWire FD";
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
    spa_zero(stream_hook_);
    stream_events_ = new struct pw_stream_events();
    spa_zero(*stream_events_);
    stream_events_->version = PW_VERSION_STREAM_EVENTS;
    stream_events_->state_changed = pw_on_stream_state_changed;
    stream_events_->param_changed = pw_on_stream_param_changed;
    stream_events_->process = pw_on_stream_process;
    pw_stream_add_listener(pw_stream_, &stream_hook_, stream_events_, this);

    return true;
}

bool PipeWireScreencap::pw_connect_stream(uint32_t node_id)
{
    // Build format negotiation parameters: accept any resolution/framerate
    uint8_t buffer[4096];
    struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));
    struct spa_video_info_raw video_info = {
        .format = SPA_VIDEO_FORMAT_BGRA,
        .size = SPA_RECTANGLE(0, 0),
        .framerate = { 0, 1 },
    };
    const struct spa_pod* params[1];
    params[0] = spa_format_video_raw_build(&b, SPA_PARAM_EnumFormat, &video_info);

    constexpr auto stream_flags =
        static_cast<enum pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_DONT_RECONNECT | PW_STREAM_FLAG_MAP_BUFFERS);

    // Start the bg thread BEFORE pw_stream_connect
    if (pw_thread_loop_start(pw_thread_loop_) < 0) {
        LogError << "Failed to start pw_thread_loop";
        return false;
    }

    pw_thread_loop_lock(pw_thread_loop_);
    int ret = pw_stream_connect(pw_stream_, PW_DIRECTION_INPUT, node_id, stream_flags, params, 1);
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

    // Extract width/height from spa_pod using manual property iteration.
    // spa_format_video_raw_parse can fail on portal-private PW instances.
    uint32_t width = static_cast<uint32_t>(self->screen_width_);
    uint32_t height = static_cast<uint32_t>(self->screen_height_);

    if (param) {
        const struct spa_pod_prop* prop;
        SPA_POD_OBJECT_FOREACH((struct spa_pod_object*)param, prop)
        {
            if (prop->key == SPA_FORMAT_VIDEO_size) {
                const struct spa_pod* val = &prop->value;
                if (spa_pod_is_rectangle(val)) {
                    struct spa_rectangle* rect = static_cast<struct spa_rectangle*>(SPA_POD_BODY(val));
                    if (rect->width > 0 && rect->height > 0) {
                        width = rect->width;
                        height = rect->height;
                    }
                }
            }
        }
    }

    self->frame_width_ = static_cast<int>(width);
    self->frame_height_ = static_cast<int>(height);

    // Reply with SPA_PARAM_Buffers to complete format negotiation
    if (self->pw_stream_) {
        uint8_t pod_buffer[1024];
        struct spa_pod_builder b = SPA_POD_BUILDER_INIT(pod_buffer, sizeof(pod_buffer));
        const struct spa_pod* buf_params[1];

        buf_params[0] = static_cast<spa_pod*>(spa_pod_builder_add_object(
            &b,
            SPA_TYPE_OBJECT_ParamBuffers,
            SPA_PARAM_Buffers,
            SPA_PARAM_BUFFERS_buffers,
            SPA_POD_CHOICE_RANGE_Int(kPWBufferCountDefault, kPWBufferCountMin, kPWBufferCountMax),
            SPA_PARAM_BUFFERS_blocks,
            SPA_POD_Int(kPWBufferBlocks),
            SPA_PARAM_BUFFERS_size,
            SPA_POD_Int(static_cast<int32_t>(width * height * kBytesPerPixel)),
            SPA_PARAM_BUFFERS_stride,
            SPA_POD_Int(static_cast<int32_t>(width * kBytesPerPixel)),
            SPA_PARAM_BUFFERS_align,
            SPA_POD_Int(kPWBufferAlign)));

        if (pw_stream_update_params(self->pw_stream_, buf_params, 1) < 0) {
            LogError << "pw_stream_update_params failed";
        }

        pw_stream_set_active(self->pw_stream_, true);
    }
}

void PipeWireScreencap::pw_on_stream_process(void* data)
{
    auto* self = static_cast<PipeWireScreencap*>(data);

    struct pw_buffer* buf = pw_stream_dequeue_buffer(self->pw_stream_);
    if (!buf) {
        return;
    }

    struct spa_buffer* spa_buf = buf->buffer;
    if (!spa_buf || spa_buf->n_datas < 1) {
        pw_stream_queue_buffer(self->pw_stream_, buf);
        return;
    }

    cv::Mat frame;
    if (self->process_frame(spa_buf, frame)) {
        {
            std::lock_guard<std::mutex> lock(self->frame_mutex_);
            self->latest_frame_ = std::move(frame);
            self->frame_available_ = true;
        }
        self->frame_cv_.notify_one();
    }

    pw_stream_queue_buffer(self->pw_stream_, buf);
}

// ===========================================================================
// Frame processing
// ===========================================================================

bool PipeWireScreencap::process_frame(const struct spa_buffer* spa_buf, cv::Mat& out_image)
{
    if (!spa_buf || spa_buf->n_datas < 1) {
        return false;
    }

    const struct spa_data& data = spa_buf->datas[0];
    if (!data.chunk || data.chunk->size == 0) {
        return false;
    }

    // Obtain a CPU-accessible pointer to the buffer data
    void* mapped_ptr = data.data;
    bool need_unmap = false;
    size_t map_size = 0;

    if (!mapped_ptr) {
        if (data.type == SPA_DATA_MemFd && data.fd >= 0) {
            map_size = data.maxsize > 0 ? data.maxsize : data.chunk->size;
            if (map_size > 0) {
                mapped_ptr = mmap(nullptr, map_size, PROT_READ, MAP_PRIVATE, data.fd, 0);
                if (mapped_ptr == MAP_FAILED) {
                    LogError << "mmap of PipeWire buffer failed: " << strerror(errno);
                    return false;
                }
                need_unmap = true;
            }
        }
        else if (data.type == SPA_DATA_DmaBuf) {
            // DMABuf cannot be directly mapped via mmap; if a valid frame is
            // already cached, skip silently and keep the last good frame.
            {
                std::lock_guard<std::mutex> lock(frame_mutex_);
                if (!latest_frame_.empty()) {
                    return false;
                }
            }
            if (!dmabuf_warned_) {
                dmabuf_warned_ = true;
                LogWarn << "DMABUF buffer type received, dropping frame (EGL import not supported)";
            }
            return false;
        }
        else {
            LogError << "Unsupported buffer type: " << data.type;
            return false;
        }
    }

    if (!mapped_ptr) {
        return false;
    }

    // Determine actual frame dimensions
    int actual_width = frame_width_;
    int actual_height = frame_height_;

    if (actual_width <= 0 || actual_height <= 0) {
        uint32_t stride = data.chunk->stride;
        size_t data_size = data.chunk->size;

        if (stride > 0 && data_size > 0) {
            actual_width = infer_dimension_from_stride(stride, data_size);
            if (actual_width > 0) {
                actual_height = static_cast<int>(data_size / stride);
            }
        }
    }

    // Last resort: configured screen dimensions
    if (actual_width <= 0 || actual_height <= 0) {
        actual_width = screen_width_;
        actual_height = screen_height_;
    }

    if (actual_width <= 0 || actual_height <= 0) {
        LogError << "Unable to determine frame dimensions";
        if (need_unmap) {
            munmap(mapped_ptr, map_size);
        }
        return false;
    }

    uint32_t stride = data.chunk->stride;
    if (stride == 0) {
        stride = static_cast<uint32_t>(actual_width * kBytesPerPixel);
    }

    // Create OpenCV Mat header wrapping the buffer (no copy until cvtColor)
    cv::Mat raw(actual_height, actual_width, CV_8UC4, mapped_ptr, stride);
    if (raw.empty()) {
        if (need_unmap) {
            munmap(mapped_ptr, map_size);
        }
        return false;
    }

    // Convert BGRA -> BGR (this copies the data)
    cv::Mat bgr_frame;
    cv::cvtColor(raw, bgr_frame, cv::COLOR_BGRA2BGR);

    if (need_unmap) {
        munmap(mapped_ptr, map_size);
    }

    // Scale to target dimensions if needed
    if (bgr_frame.size().width != screen_width_ || bgr_frame.size().height != screen_height_) {
        cv::resize(bgr_frame, out_image, cv::Size(screen_width_, screen_height_));
    }
    else {
        out_image = std::move(bgr_frame);
    }

    return true;
}

int PipeWireScreencap::infer_dimension_from_stride(uint32_t stride, size_t /*data_size*/) const
{
    // For BGRA, each pixel is 4 bytes, so width = stride / 4
    if (stride > 0) {
        return static_cast<int>(stride / kBytesPerPixel);
    }
    return 0;
}

MAA_CTRL_UNIT_NS_END
