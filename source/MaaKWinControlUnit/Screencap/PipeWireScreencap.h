#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>

#include <spa/utils/hook.h>

#include <pipewire/stream.h>

#include <opencv2/core.hpp>

#include "Common/Conf.h"

/* Forward-declare PipeWire types (they are C types, not in any C++ namespace) */
struct pw_thread_loop;
struct pw_loop;
struct pw_context;
struct pw_core;
struct pw_stream;
struct spa_buffer;
struct spa_pod;

/* Forward-declare D-Bus types (only used as pointers in method signatures) */
struct DBusConnection;

MAA_CTRL_UNIT_NS_BEGIN

/**
 * @brief PipeWire screen capture via xdg-desktop-portal (KDE/KWin).
 *
 * Captures the entire monitor framebuffer using the ScreenCast portal.
 * The workflow is:
 *   1. D-Bus handshake with xdg-desktop-portal to create a session,
 *      select sources, start streaming, and obtain a PipeWire FD.
 *   2. Connect to KWin's private PipeWire instance via the portal FD.
 *   3. Negotiate format and receive frames via PipeWire callbacks.
 *   4. Convert BGRA → BGR and resize to target screen dimensions.
 *
 * Thread safety: open()/close()/screencap() are NOT safe for concurrent
 * calls, but screencap() may be called from a different thread than
 * open()/close() as long as they are serialised by the caller.
 */
class PipeWireScreencap
{
public:
    PipeWireScreencap();
    ~PipeWireScreencap();

    PipeWireScreencap(const PipeWireScreencap&) = delete;
    PipeWireScreencap& operator=(const PipeWireScreencap&) = delete;

    void set_screen_size(int width, int height);

    bool open();
    void close();
    bool connected() const;

    bool screencap(cv::Mat& image);

private:
    /* ---- Internal cleanup ---- */
    void close_internal();

    /* ---- D-Bus (xdg-desktop-portal) ---- */
    bool dbus_create_session();
    bool dbus_select_sources();
    bool dbus_start_stream();
    bool dbus_open_pipewire_remote(DBusConnection* conn);

    /* ---- PipeWire ---- */
    bool pw_init();
    bool pw_create_stream();
    bool pw_connect_stream(uint32_t node_id);

    /* ---- PipeWire callbacks (static) ---- */
    static void pw_on_core_error(void* data, uint32_t id, int seq, int res, const char* message);
    static void pw_on_stream_state_changed(void* data, enum pw_stream_state old_state,
                                           enum pw_stream_state new_state, const char* error);
    static void pw_on_stream_param_changed(void* data, uint32_t id, const struct spa_pod* param);
    static void pw_on_stream_process(void* data);

    /* ---- Frame processing ---- */
    bool process_frame(const struct spa_buffer* spa_buf, cv::Mat& out_image);
    int infer_dimension_from_stride(uint32_t stride, size_t data_size) const;

    /* ---- Internal state ---- */
    std::atomic<bool> connected_{ false };
    bool open_attempted_ = false;

    int screen_width_ = 0;
    int screen_height_ = 0;

    /* D-Bus state */
    DBusConnection* dbus_connection_ = nullptr;
    std::string dbus_session_handle_;
    uint32_t pipewire_node_id_ = 0;
    int pipewire_fd_ = -1;

    /* PipeWire objects */
    pw_thread_loop* pw_thread_loop_ = nullptr;
    pw_loop* pw_loop_ = nullptr;
    pw_context* pw_context_ = nullptr;
    pw_core* pw_core_ = nullptr;
    pw_stream* pw_stream_ = nullptr;

    /* PipeWire core hook (must outlive pw_core_add_listener) */
    spa_hook core_hook_ = { };

    /* PipeWire stream hook (embedded, must outlive pw_stream_add_listener) */
    spa_hook stream_hook_ = { };

    /* Heap-allocated event vtables (must outlive the listener) */
    pw_stream_events* stream_events_ = nullptr;

    /* Negotiated frame dimensions (set by param_changed callback) */
    int frame_width_ = 0;
    int frame_height_ = 0;

    /* DMABUF warning (log only once to avoid flooding) */
    bool dmabuf_warned_ = false;

    /* Latest frame data (protected by frame_mutex_) */
    std::mutex frame_mutex_;
    cv::Mat latest_frame_;
    bool frame_available_ = false;
    std::condition_variable frame_cv_;
};

MAA_CTRL_UNIT_NS_END
