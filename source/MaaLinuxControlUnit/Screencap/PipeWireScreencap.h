#pragma once

#include "Base/UnitBase.h"

#include <atomic>
#include <string>

#include <memory>

#include <spa/utils/hook.h>

#include <pipewire/stream.h>
#include <spa/param/video/raw.h>

#include <opencv2/core.hpp>

#include "Common/Conf.h"

/* Forward-declare PipeWire types (they are C types, not in any C++ namespace) */
struct pw_thread_loop;
struct pw_context;
struct pw_core;
struct pw_stream;
struct spa_buffer;
struct spa_pod;

MAA_CTRL_UNIT_NS_BEGIN

/**
 * @brief PipeWire screen capture.
 *
 * Two connection modes:
 *   1. Portal FD: the caller hands over a PipeWire FD (e.g. from the ScreenCast portal, the
 *      KDE/KWin path) and the stream node ID.
 *   2. Session daemon: no FD; connects to the session PipeWire daemon and attaches to the given
 *      node (e.g. a gamescope Video/Source node).
 *
 * Frame dimensions are always taken from the negotiated stream format, never from the caller.
 *
 * Thread safety: init()/close()/screencap() are NOT safe for concurrent
 * calls, but screencap() may be called from a different thread than
 * init()/close() as long as they are serialised by the caller.
 *
 * Cross-thread state (latest_frame_, frame dimensions, frame_wanted_)
 * follows the pw_thread_loop discipline: consumer-side code holds the
 * loop lock when touching it, and callbacks run on the loop thread, so
 * the loop stops while the lock is held. Handshakes use
 * pw_thread_loop_timed_wait()/pw_thread_loop_signal().
 */
class PipeWireScreencap : public ScreencapBase
{
public:
    explicit PipeWireScreencap(int pipewire_fd, uint32_t pipewire_node_id);
    ~PipeWireScreencap() override;

    PipeWireScreencap(const PipeWireScreencap&) = delete;
    PipeWireScreencap& operator=(const PipeWireScreencap&) = delete;

    bool init() override;
    void close();
    bool connected() const;

    bool screencap(/*out*/ cv::Mat& image) override;

    void inactive() override;

private:
    /* ---- Internal cleanup ---- */
    void close_internal();

    /* ---- PipeWire ---- */
    bool pw_init();
    bool pw_create_stream();
    bool pw_connect_stream(uint32_t node_id);

    /* ---- PipeWire callbacks (static) ---- */
    static void pw_on_core_error(void* data, uint32_t id, int seq, int res, const char* message);
    static void pw_on_stream_state_changed(void* data, enum pw_stream_state old_state, enum pw_stream_state new_state, const char* error);
    static void pw_on_stream_param_changed(void* data, uint32_t id, const struct spa_pod* param);
    static void pw_on_stream_process(void* data);

    /* ---- Frame processing ---- */
    /* 需持 loop 锁调用: 置位 frame_wanted_ 等新帧, 超时取消请求 */
    bool wait_for_frame_locked(int timeout_sec);
    bool copy_raw_frame(const struct spa_buffer* spa_buf);

    /* PipeWire state */
    int pipewire_fd_ = -1;
    uint32_t pipewire_node_id_ = 0;

    /* ---- Internal state ---- */
    std::atomic<bool> connected_ { false };

    /* PipeWire objects */
    pw_thread_loop* pw_thread_loop_ = nullptr;
    pw_context* pw_context_ = nullptr;
    pw_core* pw_core_ = nullptr;
    pw_stream* pw_stream_ = nullptr;

    /* PipeWire core hook (must outlive pw_core_add_listener) */
    spa_hook core_hook_ = { };

    /* PipeWire stream hook (embedded, must outlive pw_stream_add_listener) */
    spa_hook stream_hook_ = { };

    /* Negotiated frame dimensions (set by param_changed callback) */
    int frame_width_ = 0;
    int frame_height_ = 0;
    /* Negotiated SPA video format */
    enum spa_video_format frame_format_ = SPA_VIDEO_FORMAT_UNKNOWN;

    /* 最新一帧 (协商格式): 仅回调写入; 超时回退时交付它的转换结果 */
    cv::Mat latest_frame_;
    /* 帧请求握手: screencap 置位等帧, 回调拷贝成功后复位并 signal */
    bool frame_wanted_ = false;
    /* 连续帧超时计数: 静默死亡的 producer 无事件可依, 只能靠它兜底判定 */
    int frame_timeout_count_ = 0;
};

MAA_CTRL_UNIT_NS_END
