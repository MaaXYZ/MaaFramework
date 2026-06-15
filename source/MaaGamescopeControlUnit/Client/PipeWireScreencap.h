#pragma once

#include <chrono>
#include <cstdint>

#include <opencv2/core/mat.hpp>
#include <pipewire/pipewire.h>
#include <spa/param/video/format-utils.h>

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

class PipeWireScreencap
{
public:
    explicit PipeWireScreencap(uint32_t node_id);
    ~PipeWireScreencap();

    PipeWireScreencap(const PipeWireScreencap&) = delete;
    PipeWireScreencap& operator=(const PipeWireScreencap&) = delete;

    bool init();
    bool screencap(cv::Mat& image, std::chrono::milliseconds timeout = std::chrono::seconds(5));
    void shutdown();

    bool connected() const { return connected_; }

    int width() const { return width_; }

    int height() const { return height_; }

private:
    static void on_param_changed(void* data, uint32_t id, const struct spa_pod* param);
    static void on_process(void* data);

    uint32_t node_id_;

    struct pw_thread_loop* tloop_ = nullptr;
    struct pw_stream* stream_ = nullptr;

    struct pw_stream_events stream_events_ = { .version = PW_VERSION_STREAM_EVENTS,
                                              .param_changed = on_param_changed,
                                              .process = on_process };

    bool connected_ = false;

    int width_ = 0;
    int height_ = 0;
    enum spa_video_format video_format_ = SPA_VIDEO_FORMAT_UNKNOWN;
    int channels_ = 0;

    cv::Mat latest_frame_;
    bool capture_requested_ = false;
};

MAA_CTRL_UNIT_NS_END
