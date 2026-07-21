#include "WlrScreencap.h"

#include <opencv2/imgproc.hpp>

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

std::optional<cv::Mat> WlrScreencap::screencap()
{
    if (!client_->connected()) {
        return std::nullopt;
    }

    std::unique_ptr<zwlr_screencopy_frame_v1> screencopy_frame;
    screencopy_frame.reset(zwlr_screencopy_manager_v1_capture_output(screencopy_manager_.get(), 0, output_.get()));

    zwlr_screencopy_frame_v1_listener frame_listener = { };
    frame_listener.buffer = [](void* data, zwlr_screencopy_frame_v1* frame, uint32_t format, uint32_t w, uint32_t h, uint32_t stride) {
        LogTrace << "Received new buffer: " << VAR(format) << VAR(w) << VAR(h) << VAR(stride);
        const auto self = static_cast<WlrScreencap*>(data);
        if (!self->check_buffer(format, w, h, stride)) {
            LogDebug << "Buffer format changed, recreating";
            if (!self->create_buffer(format, w, h, stride)) {
                LogError << "Failed to prepare buffer";
                return;
            }
        }
        zwlr_screencopy_frame_v1_copy(frame, self->buffer_obj_.get());
    };
    frame_listener.ready =
        [](void* data, zwlr_screencopy_frame_v1* frame, uint32_t tv_sec_hi, uint32_t tv_sec_lo, [[maybe_unused]] uint32_t tv_nsec) {
            std::ignore = frame; // do not destroy manual
            const auto self = static_cast<WlrScreencap*>(data);
            int64_t seconds = (static_cast<int64_t>(tv_sec_hi) << 32) + tv_sec_lo;
            LogTrace << "Captured frame on " << VAR(seconds);
            self->capture_waiting_ = false;
            self->capture_successful_ = true;
        };
    frame_listener.failed = [](void* data, zwlr_screencopy_frame_v1* frame) {
        std::ignore = frame; // do not destroy manual
        const auto self = static_cast<WlrScreencap*>(data);
        self->capture_waiting_ = false;
        self->capture_successful_ = false;
        LogError << "Failed to capture frame";
    };
    frame_listener.damage = [](void* data, zwlr_screencopy_frame_v1* frame, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
        std::ignore = data;
        std::ignore = frame;
        LogTrace << "Buffer damage: " << VAR(x) << VAR(y) << VAR(w) << VAR(h);
        // nop
    };
    frame_listener.buffer_done = [](void* data, zwlr_screencopy_frame_v1* frame) {
        std::ignore = data;
        std::ignore = frame;
        LogTrace << "All buffer sent";
        // nop
    };
    frame_listener.linux_dmabuf = [](void* data, zwlr_screencopy_frame_v1* frame, uint32_t format, uint32_t w, uint32_t h) {
        std::ignore = data;
        std::ignore = frame;
        LogTrace << "Received new dmabuf buffer: " << VAR(format) << VAR(w) << VAR(h);
        // nop
    };
    frame_listener.flags = [](void* data, struct zwlr_screencopy_frame_v1* frame, uint32_t flags) {
        std::ignore = data;
        std::ignore = frame;
        LogTrace << "Capture flag: " << VAR(flags);
    };
    capture_waiting_ = true;
    capture_successful_ = false;
    zwlr_screencopy_frame_v1_add_listener(screencopy_frame.get(), &frame_listener, this);

    while (capture_waiting_) {
        if (!client_->process_requests()) {
            return std::nullopt;
        }
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
        return std::nullopt;
    }

    cv::Mat raw(buffer_height_, buffer_width_, cv_format, buffer_->ptr());
    cv::Mat target;
    if (cvt_mode.has_value()) {
        LogDebug << "Converting buffer" << VAR(buffer_format_) << VAR(cvt_mode.value());
        cv::cvtColor(raw, target, cvt_mode.value());
    }
    std::optional ret(target);
    return ret;
}

bool WlrScreencap::check_buffer(int format, int width, int height, int stride) const
{
    if (!buffer_obj_ && !buffer_) {
        return false;
    }
    if (width != buffer_width_ || height != buffer_height_ || stride != buffer_stride_ || format != buffer_format_) {
        return false;
    }
    return true;
}

bool WlrScreencap::create_buffer(int format, int width, int height, int stride)
{
    if (!client_->connected()) {
        return false;
    }

    if (!close_buffer()) {
        LogError << "Failed to close old buffer";
    }
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

bool WlrScreencap::close_buffer()
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
