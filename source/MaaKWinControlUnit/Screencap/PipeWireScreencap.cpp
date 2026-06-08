#if defined(__linux__) && !defined(__ANDROID__)

#include "PipeWireScreencap.h"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <string>

#include <sys/mman.h>
#include <unistd.h>

#include <dbus/dbus.h>
#include <pipewire/pipewire.h>
#include <pipewire/stream.h>
#include <pipewire/thread-loop.h>
#include <spa/param/video/raw.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/buffers.h>
#include <spa/pod/builder.h>
#include <spa/pod/iter.h>

#include <chrono>

#include <opencv2/imgproc.hpp>

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

// ---------------------------------------------------------------------------
// D-Bus constants for xdg-desktop-portal ScreenCast
// ---------------------------------------------------------------------------
static constexpr const char* kPortalBusName = "org.freedesktop.portal.Desktop";
static constexpr const char* kPortalPath = "/org/freedesktop/portal/desktop";
static constexpr const char* kPortalInterface = "org.freedesktop.portal.ScreenCast";
static constexpr const char* kPortalRequestInterface = "org.freedesktop.portal.Request";
static constexpr const char* kSessionInterface = "org.freedesktop.portal.Session";

/// Default timeout for D-Bus pending calls (milliseconds).
static constexpr int kDBusCallTimeoutMs = 10000;

/// Timeout for OpenPipeWireRemote (ms).
static constexpr int kOpenPipeWireTimeoutMs = 5000;

/// Timeout for waiting for the Start Response signal (seconds).
static constexpr int kStartResponseTimeoutSec = 30;

/// D-Bus polling interval when waiting for signals (milliseconds).
static constexpr int kDBusPollIntervalMs = 100;

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

// ---------------------------------------------------------------------------
// Helper: wait for a D-Bus Response signal on a given request path.
//
// Polls up to @p timeout_sec seconds, draining messages from the connection.
// Returns true if the Response signal was received, storing the response code
// in @p response. If @p extract_key is non-null, extracts the corresponding
// string value from the results dict into @p out_value.
// ---------------------------------------------------------------------------
static bool dbus_wait_for_signal(DBusConnection* conn, const std::string& request_path,
                                 uint32_t& response, const char* extract_key,
                                 std::string& out_value, int timeout_sec = kStartResponseTimeoutSec)
{
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout_sec);

    while (std::chrono::steady_clock::now() < deadline) {
        dbus_connection_flush(conn);

        // Non-blocking read with short timeout to allow deadline expiry
        if (!dbus_connection_read_write(conn, kDBusPollIntervalMs)) {
            continue;
        }

        DBusMessage* msg = dbus_connection_pop_message(conn);
        if (!msg) {
            continue;
        }

        if (dbus_message_is_signal(msg, kPortalRequestInterface, "Response")) {
            const char* path = dbus_message_get_path(msg);
            if (path && request_path == path) {
                DBusMessageIter args;
                if (!dbus_message_iter_init(msg, &args)) {
                    dbus_message_unref(msg);
                    continue;
                }

                if (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_UINT32) {
                    dbus_message_unref(msg);
                    continue;
                }
                dbus_message_iter_get_basic(&args, &response);

                // Parse results dict if needed
                if (extract_key && extract_key[0] != '\0') {
                    dbus_message_iter_next(&args);
                    if (dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_ARRAY) {
                        DBusMessageIter dict_iter;
                        dbus_message_iter_recurse(&args, &dict_iter);

                        while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY) {
                            DBusMessageIter entry;
                            dbus_message_iter_recurse(&dict_iter, &entry);

                            const char* key = nullptr;
                            dbus_message_iter_get_basic(&entry, &key);
                            dbus_message_iter_next(&entry);

                            DBusMessageIter value;
                            dbus_message_iter_recurse(&entry, &value);

                            if (key && strcmp(key, extract_key) == 0) {
                                int value_type = dbus_message_iter_get_arg_type(&value);
                                if (value_type == DBUS_TYPE_STRING || value_type == DBUS_TYPE_OBJECT_PATH) {
                                    const char* val = nullptr;
                                    dbus_message_iter_get_basic(&value, &val);
                                    if (val) {
                                        out_value = val;
                                    }
                                }
                                break;
                            }

                            dbus_message_iter_next(&dict_iter);
                        }
                    }
                }

                dbus_message_unref(msg);
                return true;
            }
        }
        else if (dbus_message_is_signal(msg, kSessionInterface, "Closed")) {
            dbus_message_unref(msg);
            return false;
        }

        dbus_message_unref(msg);
    }

    LogError << "Timeout waiting for D-Bus Response signal on" << request_path;
    return false;
}

// ---------------------------------------------------------------------------
// Helper: send a D-Bus method call via DBusPendingCall and wait for reply.
//
// Uses DBusPendingCall (not send_with_reply_and_block) to avoid consuming
// unrelated messages during the internal read loop.
// ---------------------------------------------------------------------------
static DBusMessage* dbus_call_pending(DBusConnection* conn, const char* method_name,
                                      DBusMessage* msg, int timeout_ms = kDBusCallTimeoutMs)
{
    DBusPendingCall* pending = nullptr;
    if (!dbus_connection_send_with_reply(conn, msg, &pending, timeout_ms)) {
        LogError << "D-Bus call" << method_name << ": send_with_reply failed (OOM)";
        return nullptr;
    }
    dbus_connection_flush(conn);

    if (!pending) {
        LogError << "D-Bus call" << method_name << ": pending is null (disconnected)";
        return nullptr;
    }

    dbus_pending_call_block(pending);
    DBusMessage* reply = dbus_pending_call_steal_reply(pending);
    dbus_pending_call_unref(pending);

    if (!reply) {
        LogError << "D-Bus call" << method_name << ": reply is null (timeout)";
        return nullptr;
    }

    if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
        const char* err_name = dbus_message_get_error_name(reply);
        LogError << "D-Bus call" << method_name << "returned error: " << (err_name ? err_name : "(unknown)");
        dbus_message_unref(reply);
        return nullptr;
    }

    return reply;
}

// ---------------------------------------------------------------------------
// D-Bus dictionary helpers (a{sv} entries)
// ---------------------------------------------------------------------------
static void dbus_append_dict_entry_string(DBusMessageIter* dict, const char* key, const char* value)
{
    DBusMessageIter entry, variant;
    dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "s", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &value);
    dbus_message_iter_close_container(&entry, &variant);
    dbus_message_iter_close_container(dict, &entry);
}

static void dbus_append_dict_entry_uint32(DBusMessageIter* dict, const char* key, uint32_t value)
{
    DBusMessageIter entry, variant;
    dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "u", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_UINT32, &value);
    dbus_message_iter_close_container(&entry, &variant);
    dbus_message_iter_close_container(dict, &entry);
}

static void dbus_append_dict_entry_bool(DBusMessageIter* dict, const char* key, bool value)
{
    uint32_t dbus_bool_val = value ? 1 : 0;
    DBusMessageIter entry, variant;
    dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "b", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &dbus_bool_val);
    dbus_message_iter_close_container(&entry, &variant);
    dbus_message_iter_close_container(dict, &entry);
}

// ===========================================================================
// PipeWireScreencap implementation
// ===========================================================================

PipeWireScreencap::PipeWireScreencap()
{
    // PipeWire global init is safe to call multiple times (uses an internal refcount).
    ::pw_init(nullptr, nullptr);
}

PipeWireScreencap::~PipeWireScreencap()
{
    close();
}

void PipeWireScreencap::set_screen_size(int width, int height)
{
    screen_width_ = width;
    screen_height_ = height;
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
    frame_width_ = 0;
    frame_height_ = 0;
    pipewire_node_id_ = 0;
    pipewire_fd_ = -1;
    dbus_session_handle_.clear();
    if (dbus_connection_) {
        dbus_connection_unref(dbus_connection_);
        dbus_connection_ = nullptr;
    }

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

    // Step 1: D-Bus handshake with xdg-desktop-portal.
    // This includes CreateSession, SelectSources, and Start (which shows
    // the KDE authorization dialog). Timeout is kStartResponseTimeoutSec.
    if (!dbus_create_session() || !dbus_select_sources() || !dbus_start_stream()) {
        // dbus_* methods already logged errors
        close_internal();
        return false;
    }

    // Step 2: Initialize PipeWire (connect core, create stream, connect)
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

bool PipeWireScreencap::screencap(cv::Mat& image)
{
    // Lazy init: try once on first screencap() call
    if (!connected_ && !open_attempted_) {
        open_attempted_ = true;
        if (!open()) {
            return false;
        }
    }

    if (!connected_) {
        return false;
    }

    // Wait for the first frame with a timeout.
    // Subsequent calls return the cached frame immediately without waiting.
    std::unique_lock<std::mutex> lock(frame_mutex_);
    if (latest_frame_.empty()) {
        if (!frame_cv_.wait_for(lock, std::chrono::seconds(2),
                                [this]() { return !latest_frame_.empty(); })) {
            LogError << "Timeout waiting for first PipeWire frame";
            return false;
        }
    }

    latest_frame_.copyTo(image);
    return true;
}

// ===========================================================================
// D-Bus implementation
// ===========================================================================

bool PipeWireScreencap::dbus_open_pipewire_remote(DBusConnection* conn)
{
    // OpenPipeWireRemote(session_handle, a{sv}) -> (h)
    DBusMessage* msg = dbus_message_new_method_call(
        kPortalBusName, kPortalPath, kPortalInterface, "OpenPipeWireRemote");
    if (!msg) {
        LogError << "Failed to create OpenPipeWireRemote message";
        return false;
    }

    DBusMessageIter iter, dict;
    dbus_message_iter_init_append(msg, &iter);

    const char* session_path = dbus_session_handle_.c_str();
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH, &session_path);

    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
    dbus_message_iter_close_container(&iter, &dict);

    DBusPendingCall* pending = nullptr;
    if (!dbus_connection_send_with_reply(conn, msg, &pending, kOpenPipeWireTimeoutMs)) {
        LogError << "OpenPipeWireRemote: send_with_reply failed (OOM)";
        dbus_message_unref(msg);
        return false;
    }
    dbus_connection_flush(conn);
    dbus_message_unref(msg);

    if (!pending) {
        LogError << "OpenPipeWireRemote: pending is null (disconnected)";
        return false;
    }

    dbus_pending_call_block(pending);
    DBusMessage* reply = dbus_pending_call_steal_reply(pending);
    dbus_pending_call_unref(pending);

    if (!reply) {
        LogError << "OpenPipeWireRemote: reply is null (timeout)";
        return false;
    }

    if (dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
        const char* err_name = dbus_message_get_error_name(reply);
        LogError << "OpenPipeWireRemote error: " << (err_name ? err_name : "(unknown)");
        dbus_message_unref(reply);
        return false;
    }

    // Extract Unix FD — must dup() before unref as libdbus closes the FD on free
    DBusError err;
    dbus_error_init(&err);

    int raw_fd = -1;
    if (!dbus_message_get_args(reply, &err,
                               DBUS_TYPE_UNIX_FD, &raw_fd,
                               DBUS_TYPE_INVALID)) {
        LogError << "OpenPipeWireRemote: no FD in reply: " << (err.message ? err.message : "");
        dbus_error_free(&err);
        dbus_message_unref(reply);
        return false;
    }
    dbus_error_free(&err);

    if (raw_fd >= 0) {
        pipewire_fd_ = dup(raw_fd);
        if (pipewire_fd_ < 0) {
            LogError << "Failed to dup PipeWire FD:" << strerror(errno);
        }
    }

    dbus_message_unref(reply);

    if (pipewire_fd_ < 0) {
        LogError << "OpenPipeWireRemote: invalid FD";
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// dbus_create_session: calls org.freedesktop.portal.ScreenCast.CreateSession
// ---------------------------------------------------------------------------
bool PipeWireScreencap::dbus_create_session()
{
    if (!dbus_connection_) {
        DBusError err;
        dbus_error_init(&err);
        dbus_connection_ = dbus_bus_get(DBUS_BUS_SESSION, &err);
        if (!dbus_connection_) {
            LogError << "Failed to connect to session bus:" << err.message;
            dbus_error_free(&err);
            return false;
        }
    }
    DBusConnection* conn = dbus_connection_;

    DBusMessage* msg = dbus_message_new_method_call(
        kPortalBusName, kPortalPath, kPortalInterface, "CreateSession");
    if (!msg) {
        LogError << "Failed to create CreateSession message";
        return false;
    }

    DBusMessageIter iter, dict;
    dbus_message_iter_init_append(msg, &iter);
    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
    // session_handle_token is REQUIRED by xdg-desktop-portal
    dbus_append_dict_entry_string(&dict, "session_handle_token", "maa_session");
    dbus_message_iter_close_container(&iter, &dict);

    DBusMessage* reply = dbus_call_pending(conn, "CreateSession", msg);
    dbus_message_unref(msg);
    if (!reply) {
        return false;
    }

    // Parse reply: (o request_path)
    DBusMessageIter args;
    std::string create_request_path;
    if (dbus_message_iter_init(reply, &args) &&
        dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_OBJECT_PATH) {
        const char* p = nullptr;
        dbus_message_iter_get_basic(&args, &p);
        if (p) create_request_path = p;
    }

    if (create_request_path.empty()) {
        LogError << "CreateSession: missing request path";
        dbus_message_unref(reply);
        return false;
    }

    // Wait for Response signal with session_handle
    uint32_t response_code = 0;
    std::string session_handle;
    if (!dbus_wait_for_signal(conn, create_request_path, response_code,
                              "session_handle", session_handle)) {
        LogError << "CreateSession: no response signal";
        dbus_message_unref(reply);
        return false;
    }

    if (response_code != 0) {
        LogError << "CreateSession failed, code=" << response_code;
        dbus_message_unref(reply);
        return false;
    }

    if (session_handle.empty()) {
        LogError << "CreateSession: missing session_handle";
        dbus_message_unref(reply);
        return false;
    }

    dbus_session_handle_ = session_handle;
    dbus_message_unref(reply);
    return true;
}

// ---------------------------------------------------------------------------
// dbus_select_sources: calls org.freedesktop.portal.ScreenCast.SelectSources
// ---------------------------------------------------------------------------
bool PipeWireScreencap::dbus_select_sources()
{
    if (!dbus_connection_) {
        LogError << "D-Bus connection not established (CreateSession must be called first)";
        return false;
    }
    DBusConnection* conn = dbus_connection_;

    DBusMessage* msg = dbus_message_new_method_call(
        kPortalBusName, kPortalPath, kPortalInterface, "SelectSources");
    if (!msg) {
        LogError << "Failed to create SelectSources message";
        return false;
    }

    DBusMessageIter iter, dict;
    dbus_message_iter_init_append(msg, &iter);

    const char* session_path = dbus_session_handle_.c_str();
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH, &session_path);

    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
    dbus_append_dict_entry_string(&dict, "handle_token", "maa_select");
    dbus_append_dict_entry_uint32(&dict, "types", 1);   // MONITOR
    dbus_append_dict_entry_bool(&dict, "multiple", false);
    dbus_message_iter_close_container(&iter, &dict);

    DBusMessage* reply = dbus_call_pending(conn, "SelectSources", msg);
    dbus_message_unref(msg);
    if (!reply) {
        return false;
    }

    DBusMessageIter args;
    std::string select_request_path;
    if (dbus_message_iter_init(reply, &args) &&
        dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_OBJECT_PATH) {
        const char* p = nullptr;
        dbus_message_iter_get_basic(&args, &p);
        if (p) select_request_path = p;
    }

    if (select_request_path.empty()) {
        LogError << "SelectSources: missing request path";
        dbus_message_unref(reply);
        return false;
    }

    uint32_t select_response = 0;
    std::string unused;
    if (!dbus_wait_for_signal(conn, select_request_path, select_response, nullptr, unused)) {
        LogError << "SelectSources: no response signal";
        dbus_message_unref(reply);
        return false;
    }

    if (select_response != 0) {
        LogError << "SelectSources failed, code=" << select_response;
        dbus_message_unref(reply);
        return false;
    }

    dbus_message_unref(reply);
    return true;
}

// ---------------------------------------------------------------------------
// dbus_start_stream: calls org.freedesktop.portal.ScreenCast.Start and
// parses the response for the PipeWire node_id.
// ---------------------------------------------------------------------------
bool PipeWireScreencap::dbus_start_stream()
{
    if (!dbus_connection_) {
        LogError << "D-Bus connection not established (CreateSession must be called first)";
        return false;
    }
    DBusConnection* conn = dbus_connection_;

    DBusMessage* msg = dbus_message_new_method_call(
        kPortalBusName, kPortalPath, kPortalInterface, "Start");
    if (!msg) {
        LogError << "Failed to create Start message";
        return false;
    }

    DBusMessageIter iter, dict;
    dbus_message_iter_init_append(msg, &iter);

    const char* session_path = dbus_session_handle_.c_str();
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_OBJECT_PATH, &session_path);

    const char* parent_window = "";
    dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &parent_window);

    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
    dbus_append_dict_entry_string(&dict, "handle_token", "maa_start");
    dbus_message_iter_close_container(&iter, &dict);

    DBusMessage* reply = dbus_call_pending(conn, "Start", msg);
    dbus_message_unref(msg);
    if (!reply) {
        return false;
    }

    DBusMessageIter args;
    std::string start_request_path;
    if (dbus_message_iter_init(reply, &args) &&
        dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_OBJECT_PATH) {
        const char* p = nullptr;
        dbus_message_iter_get_basic(&args, &p);
        if (p) start_request_path = p;
    }

    if (start_request_path.empty()) {
        LogError << "Start: missing request path";
        dbus_message_unref(reply);
        return false;
    }

    // Add match rule so the D-Bus daemon delivers the Response signal
    DBusError err;
    dbus_error_init(&err);
    std::string match_rule = "type='signal',interface='" + std::string(kPortalRequestInterface)
        + "',path='" + start_request_path + "'";
    dbus_bus_add_match(conn, match_rule.c_str(), &err);
    if (dbus_error_is_set(&err)) {
        dbus_error_free(&err);
    }

    // Blocking loop waiting for user to authorise screen sharing
    bool got_start_response = false;
    uint32_t response_code = 0;
    pipewire_node_id_ = 0;

    {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(kStartResponseTimeoutSec);

        while (!got_start_response && std::chrono::steady_clock::now() < deadline) {
            dbus_connection_read_write(conn, kDBusPollIntervalMs);

            DBusMessage* sig_msg = nullptr;
            while ((sig_msg = dbus_connection_pop_message(conn)) != nullptr) {
                if (dbus_message_is_signal(sig_msg, kPortalRequestInterface, "Response") &&
                    start_request_path == dbus_message_get_path(sig_msg)) {

                    DBusMessageIter sig_args;
                    dbus_message_iter_init(sig_msg, &sig_args);

                    if (dbus_message_iter_get_arg_type(&sig_args) == DBUS_TYPE_UINT32) {
                        dbus_message_iter_get_basic(&sig_args, &response_code);
                    }

                    // Parse results dict for "streams" -> node_id
                    dbus_message_iter_next(&sig_args);
                    if (dbus_message_iter_get_arg_type(&sig_args) == DBUS_TYPE_ARRAY) {
                        DBusMessageIter dict_iter;
                        dbus_message_iter_recurse(&sig_args, &dict_iter);

                        while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY) {
                            DBusMessageIter entry;
                            dbus_message_iter_recurse(&dict_iter, &entry);

                            const char* key = nullptr;
                            dbus_message_iter_get_basic(&entry, &key);
                            dbus_message_iter_next(&entry);

                            DBusMessageIter value;
                            dbus_message_iter_recurse(&entry, &value);

                            if (key && strcmp(key, "streams") == 0 &&
                                dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_ARRAY) {

                                DBusMessageIter streams_array;
                                dbus_message_iter_recurse(&value, &streams_array);

                                if (dbus_message_iter_get_arg_type(&streams_array) == DBUS_TYPE_STRUCT) {
                                    DBusMessageIter stream_struct;
                                    dbus_message_iter_recurse(&streams_array, &stream_struct);

                                    if (dbus_message_iter_get_arg_type(&stream_struct) == DBUS_TYPE_UINT32) {
                                        uint32_t node_id = 0;
                                        dbus_message_iter_get_basic(&stream_struct, &node_id);
                                        if (node_id != 0) {
                                            pipewire_node_id_ = node_id;
                                        }
                                    }
                                }
                            }

                            dbus_message_iter_next(&dict_iter);
                        }
                    }

                    got_start_response = true;
                }
                else if (dbus_message_is_signal(sig_msg, kSessionInterface, "Closed")) {
                    dbus_message_unref(sig_msg);
                    dbus_bus_remove_match(conn, match_rule.c_str(), nullptr);
                    dbus_message_unref(reply);
                    return false;
                }

                dbus_message_unref(sig_msg);
            }
        }
    }

    dbus_bus_remove_match(conn, match_rule.c_str(), nullptr);

    if (!got_start_response) {
        LogError << "Start timed out after" << kStartResponseTimeoutSec << "s (auth dialog not accepted?)";
        dbus_message_unref(reply);
        return false;
    }

    if (response_code != 0) {
        LogError << "Start rejected by user, code=" << response_code;
        dbus_message_unref(reply);
        return false;
    }

    if (pipewire_node_id_ == 0) {
        LogError << "Start: missing node_id";
        dbus_message_unref(reply);
        return false;
    }

    dbus_message_unref(reply);

    // ---- OpenPipeWireRemote after Start succeeded ----
    if (!dbus_open_pipewire_remote(conn)) {
        LogError << "OpenPipeWireRemote failed";
        return false;
    }

    return true;
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
    struct pw_properties* props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Video",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_MEDIA_ROLE, "Screen",
        nullptr);

    // Use pw_stream_new (not pw_stream_new_simple) to avoid exception
    // unwinding ABI incompatibility between clang/libc++ and PipeWire's glibc.
    pw_stream_ = pw_stream_new(pw_core_, "MaaFramework Screencap", props);
    if (!pw_stream_) {
        LogError << "Failed to create PipeWire stream: " << strerror(errno);
        if (props) pw_properties_free(props);
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

    constexpr auto stream_flags = static_cast<enum pw_stream_flags>(
        PW_STREAM_FLAG_AUTOCONNECT
        | PW_STREAM_FLAG_DONT_RECONNECT
        | PW_STREAM_FLAG_MAP_BUFFERS);

    // Start the bg thread BEFORE pw_stream_connect
    if (pw_thread_loop_start(pw_thread_loop_) < 0) {
        LogError << "Failed to start pw_thread_loop";
        return false;
    }

    pw_thread_loop_lock(pw_thread_loop_);
    int ret = pw_stream_connect(pw_stream_, PW_DIRECTION_INPUT, node_id,
                                stream_flags, params, 1);
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
    LogError << "PipeWire core error: id=" << id << " seq=" << seq
             << " res=" << res << " msg=" << (message ? message : "");

    if (id == PW_ID_CORE && res == -EPIPE) {
        self->connected_ = false;
    }
}

void PipeWireScreencap::pw_on_stream_state_changed(void* data, enum pw_stream_state old_state,
                                                   enum pw_stream_state new_state, const char* error)
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
        SPA_POD_OBJECT_FOREACH((struct spa_pod_object*)param, prop) {
            if (prop->key == SPA_FORMAT_VIDEO_size) {
                const struct spa_pod* val = &prop->value;
                if (spa_pod_is_rectangle(val)) {
                    struct spa_rectangle* rect = (struct spa_rectangle*)SPA_POD_BODY(val);
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

        buf_params[0] = (const struct spa_pod*)spa_pod_builder_add_object(&b,
            SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
            SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(
                kPWBufferCountDefault, kPWBufferCountMin, kPWBufferCountMax),
            SPA_PARAM_BUFFERS_blocks,  SPA_POD_Int(kPWBufferBlocks),
            SPA_PARAM_BUFFERS_size,    SPA_POD_Int(static_cast<int32_t>(width * height * kBytesPerPixel)),
            SPA_PARAM_BUFFERS_stride,  SPA_POD_Int(static_cast<int32_t>(width * kBytesPerPixel)),
            SPA_PARAM_BUFFERS_align,   SPA_POD_Int(kPWBufferAlign));

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
            // DMABuf cannot be directly mapped via mmap; silently drop the frame
            // and return false to let pw_on_stream_process skip this frame.
            LogWarn << "DMABUF buffer type received, dropping frame (EGL import not supported)";
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

#endif // __linux__ && !__ANDROID__
