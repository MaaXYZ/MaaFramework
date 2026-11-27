#include "ExtImageScreencap.h"

#include <optional>

#include <opencv2/imgproc.hpp>

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

namespace
{

// Bytes per pixel of a wl_shm_format supported by this unit, -1 when unsupported
int format_bpp(int format)
{
    switch (format) {
    case WL_SHM_FORMAT_XBGR8888:
    case WL_SHM_FORMAT_ABGR8888:
    case WL_SHM_FORMAT_ARGB8888:
    case WL_SHM_FORMAT_XRGB8888:
        return 4;
    case WL_SHM_FORMAT_RGB888:
    case WL_SHM_FORMAT_BGR888:
        return 3;
    default:
        return -1;
    }
}

} // namespace

bool ExtImageScreencap::screencap(cv::Mat& image)
{
    if (!client_->connected()) {
        return false;
    }
    if (!source_manager_ || !capture_manager_) {
        LogError << "Compositor doesn't support ext_image_capture_source_v1 / ext_image_copy_capture_v1 protocol";
        return false;
    }

    constraints_ready_ = false;
    session_stopped_ = false;
    capture_waiting_ = false;
    capture_successful_ = false;
    buffer_stale_ = false;
    capture_width_ = 0;
    capture_height_ = 0;
    shm_formats_.clear();

    std::unique_ptr<ext_image_capture_source_v1> source;
    std::unique_ptr<ext_image_copy_capture_session_v1> session;
    source.reset(ext_output_image_capture_source_manager_v1_create_source(source_manager_.get(), output_.get()));
    session.reset(ext_image_copy_capture_manager_v1_create_session(capture_manager_.get(), source.get(), 0));

    ext_image_copy_capture_session_v1_listener session_listener = { };
    session_listener.buffer_size = [](void* data, ext_image_copy_capture_session_v1*, uint32_t w, uint32_t h) {
        const auto self = static_cast<ExtImageScreencap*>(data);
        LogTrace << "Session buffer size: " << VAR(w) << VAR(h);
        self->capture_width_ = w;
        self->capture_height_ = h;
    };
    session_listener.shm_format = [](void* data, ext_image_copy_capture_session_v1*, uint32_t format) {
        const auto self = static_cast<ExtImageScreencap*>(data);
        LogTrace << "Session shm format: " << VAR(format);
        self->shm_formats_.push_back(format);
    };
    session_listener.dmabuf_device = []([[maybe_unused]] void* data, ext_image_copy_capture_session_v1*, wl_array* device) {
        std::ignore = device; // shm buffers only
    };
    session_listener.dmabuf_format =
        []([[maybe_unused]] void* data, ext_image_copy_capture_session_v1*, [[maybe_unused]] uint32_t format, wl_array* modifiers) {
            std::ignore = modifiers; // shm buffers only
        };
    session_listener.done = [](void* data, ext_image_copy_capture_session_v1*) {
        const auto self = static_cast<ExtImageScreencap*>(data);
        LogTrace << "Session constraints done";
        self->constraints_ready_ = true;
    };
    session_listener.stopped = [](void* data, ext_image_copy_capture_session_v1*) {
        const auto self = static_cast<ExtImageScreencap*>(data);
        LogError << "Capture session stopped";
        self->session_stopped_ = true;
    };
    ext_image_copy_capture_session_v1_add_listener(session.get(), &session_listener, this);

    while (!constraints_ready_ && !session_stopped_) {
        if (!client_->process_requests()) {
            return false;
        }
    }
    if (session_stopped_) {
        return false;
    }

    std::optional<int> selected_format;
    for (uint32_t format : shm_formats_) {
        if (format_bpp(static_cast<int>(format)) > 0) {
            selected_format = static_cast<int>(format);
            break;
        }
    }
    if (!selected_format.has_value()) {
        LogError << "No supported shm format" << VAR(shm_formats_.size());
        return false;
    }

    const int width = static_cast<int>(capture_width_);
    const int height = static_cast<int>(capture_height_);
    if (!check_buffer(selected_format.value(), width, height)) {
        LogDebug << "Buffer format changed, recreating";
        if (!create_buffer(selected_format.value(), width, height)) {
            LogError << "Failed to prepare buffer";
            return false;
        }
    }

    std::unique_ptr<ext_image_copy_capture_frame_v1> frame;
    frame.reset(ext_image_copy_capture_session_v1_create_frame(session.get()));

    ext_image_copy_capture_frame_v1_listener frame_listener = { };
    frame_listener.transform = []([[maybe_unused]] void* data, ext_image_copy_capture_frame_v1*, uint32_t transform) {
        // TODO: Other possible transform?
        LogTrace << "Capture transform: " << VAR(transform);
    };
    frame_listener.damage = []([[maybe_unused]] void* data, ext_image_copy_capture_frame_v1*, int32_t x, int32_t y, int32_t w, int32_t h) {
        LogTrace << "Buffer damage: " << VAR(x) << VAR(y) << VAR(w) << VAR(h);
        // nop
    };
    frame_listener.presentation_time =
        [](void* data, ext_image_copy_capture_frame_v1*, uint32_t tv_sec_hi, uint32_t tv_sec_lo, [[maybe_unused]] uint32_t tv_nsec) {
            std::ignore = data;
            int64_t seconds = (static_cast<int64_t>(tv_sec_hi) << 32) + tv_sec_lo;
            LogTrace << "Captured frame on " << VAR(seconds);
        };
    frame_listener.ready = [](void* data, ext_image_copy_capture_frame_v1*) {
        const auto self = static_cast<ExtImageScreencap*>(data);
        self->capture_waiting_ = false;
        self->capture_successful_ = true;
    };
    frame_listener.failed = [](void* data, ext_image_copy_capture_frame_v1*, uint32_t reason) {
        const auto self = static_cast<ExtImageScreencap*>(data);
        self->capture_waiting_ = false;
        self->capture_successful_ = false;
        LogError << "Failed to capture frame" << VAR(reason);
        if (reason == EXT_IMAGE_COPY_CAPTURE_FRAME_V1_FAILURE_REASON_BUFFER_CONSTRAINTS) {
            // The constraints changed after our buffer was created; drop it so
            // the next capture re-allocates from the new constraints.
            self->buffer_stale_ = true;
        }
    };
    ext_image_copy_capture_frame_v1_add_listener(frame.get(), &frame_listener, this);

    capture_waiting_ = true;
    ext_image_copy_capture_frame_v1_attach_buffer(frame.get(), buffer_obj_.get());
    ext_image_copy_capture_frame_v1_damage_buffer(frame.get(), 0, 0, width, height);
    ext_image_copy_capture_frame_v1_capture(frame.get());

    while (capture_waiting_) {
        if (!client_->process_requests()) {
            return false;
        }
    }

    // The protocol requires destroying the frame after ready/failed.
    frame.reset();
    session.reset();
    source.reset();

    if (buffer_stale_ && !close_buffer()) {
        LogError << "Failed to close stale buffer";
    }

    if (!capture_successful_) {
        LogError << "Failed to capture frame";
        return false;
    }

    std::optional<int> cvt_mode;
    int cv_format = -1;
    switch (buffer_format_) { // TODO: Other possible format?
    case WL_SHM_FORMAT_XBGR8888:
    case WL_SHM_FORMAT_ABGR8888:
        cvt_mode = cv::COLOR_RGBA2BGR;
        cv_format = CV_8UC4;
        break;
    case WL_SHM_FORMAT_ARGB8888:
    case WL_SHM_FORMAT_XRGB8888:
        cvt_mode = cv::COLOR_BGRA2BGR;
        cv_format = CV_8UC4;
        break;
    case WL_SHM_FORMAT_RGB888:
        // no need to cvtColor
        cvt_mode = { };
        cv_format = CV_8UC3;
        break;
    case WL_SHM_FORMAT_BGR888:
        cvt_mode = cv::COLOR_RGB2BGR;
        cv_format = CV_8UC3;
        break;
    default:
        LogError << "Unsupported wl_shm_format" << VAR(buffer_format_);
        return false;
    }

    cv::Mat raw(buffer_height_, buffer_width_, cv_format, buffer_->ptr());
    if (cvt_mode.has_value()) {
        LogDebug << "Converting buffer" << VAR(buffer_format_) << VAR(cvt_mode.value());
        cv::cvtColor(raw, image, cvt_mode.value());
    }
    else {
        raw.copyTo(image);
    }
    return true;
}

bool ExtImageScreencap::check_buffer(int format, int width, int height) const
{
    if (!buffer_obj_ && !buffer_) {
        return false;
    }
    if (width != buffer_width_ || height != buffer_height_ || format != buffer_format_) {
        return false;
    }
    return true;
}

bool ExtImageScreencap::create_buffer(int format, int width, int height)
{
    if (!client_->connected()) {
        return false;
    }

    if (!close_buffer()) {
        LogError << "Failed to close old buffer";
    }
    const int stride = width * format_bpp(format);
    buffer_ = std::make_unique<MemfdBuffer>(width, height, stride);
    if (!buffer_->available()) {
        LogError << "Failed to create shm buffer";
        return false;
    }
    buffer_format_ = format;
    buffer_width_ = width;
    buffer_height_ = height;
    buffer_stride_ = stride;
    shm_pool_.reset(wl_shm_create_pool(shm_.get(), buffer_->fd(), buffer_->size()));
    buffer_obj_.reset(wl_shm_pool_create_buffer(shm_pool_.get(), 0, width, height, stride, format));
    if (!client_->process_requests()) {
        LogError << "Failed to create buffer objects";
        return false;
    }
    return true;
}

bool ExtImageScreencap::close_buffer()
{
    if (!client_->connected()) {
        if (buffer_) {
            buffer_.reset(); // Close memfd only
        }
        return true;
    }

    if (!buffer_) {
        return true;
    }
    LogDebug << "Closing buffer" << VAR(buffer_);
    buffer_obj_.reset();
    shm_pool_.reset();
    if (!client_->process_requests()) {
        LogError << "Failed to destroy buffer objects";
        return false;
    }
    buffer_.reset();
    return true;
}

MAA_CTRL_UNIT_NS_END
