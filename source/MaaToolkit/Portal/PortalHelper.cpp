#if defined(__linux__) && !defined(ANDROID)

#include "PortalHelper.h"

#include <string>

#include <dbus/dbus.h>

#include "MaaUtils/Logger.h"

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
// Helper: wait for a D-Bus Response signal on a given request path.
//
// Polls up to @p timeout_sec seconds, draining messages from the connection.
// Returns true if the Response signal was received, storing the response code
// in @p response. If @p extract_key is non-null, extracts the corresponding
// string value from the results dict into @p out_value.
// ---------------------------------------------------------------------------
static bool dbus_wait_for_signal(
    DBusConnection* conn,
    const std::string& request_path,
    uint32_t& response,
    const char* extract_key,
    std::string& out_value,
    int timeout_sec = kStartResponseTimeoutSec)
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
static DBusMessage* dbus_call_pending(DBusConnection* conn, const char* method_name, DBusMessage* msg, int timeout_ms = kDBusCallTimeoutMs)
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

bool MaaToolkitPortalHelper::dbus_create_session()
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

    DBusMessage* msg = dbus_message_new_method_call(kPortalBusName, kPortalPath, kPortalInterface, "CreateSession");
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
    if (dbus_message_iter_init(reply, &args) && dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_OBJECT_PATH) {
        const char* p = nullptr;
        dbus_message_iter_get_basic(&args, &p);
        if (p) {
            create_request_path = p;
        }
    }

    if (create_request_path.empty()) {
        LogError << "CreateSession: missing request path";
        dbus_message_unref(reply);
        return false;
    }

    // Wait for Response signal with session_handle
    uint32_t response_code = 0;
    std::string session_handle;
    if (!dbus_wait_for_signal(conn, create_request_path, response_code, "session_handle", session_handle)) {
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
bool MaaToolkitPortalHelper::dbus_select_sources()
{
    if (!dbus_connection_) {
        LogError << "D-Bus connection not established (CreateSession must be called first)";
        return false;
    }
    DBusConnection* conn = dbus_connection_;

    DBusMessage* msg = dbus_message_new_method_call(kPortalBusName, kPortalPath, kPortalInterface, "SelectSources");
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
    dbus_append_dict_entry_uint32(&dict, "types", 1); // MONITOR
    dbus_append_dict_entry_bool(&dict, "multiple", false);
    if (!restore_token_.empty()) {
        dbus_append_dict_entry_string(&dict, "restore_token", restore_token_.c_str());
    }
    if (persist_) {
        dbus_append_dict_entry_uint32(&dict, "persist_mode", 2);
    }
    dbus_message_iter_close_container(&iter, &dict);

    DBusMessage* reply = dbus_call_pending(conn, "SelectSources", msg);
    dbus_message_unref(msg);
    if (!reply) {
        return false;
    }

    DBusMessageIter args;
    std::string select_request_path;
    if (dbus_message_iter_init(reply, &args) && dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_OBJECT_PATH) {
        const char* p = nullptr;
        dbus_message_iter_get_basic(&args, &p);
        if (p) {
            select_request_path = p;
        }
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
bool MaaToolkitPortalHelper::dbus_start_stream()
{
    if (!dbus_connection_) {
        LogError << "D-Bus connection not established (CreateSession must be called first)";
        return false;
    }
    DBusConnection* conn = dbus_connection_;

    DBusMessage* msg = dbus_message_new_method_call(kPortalBusName, kPortalPath, kPortalInterface, "Start");
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
    if (dbus_message_iter_init(reply, &args) && dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_OBJECT_PATH) {
        const char* p = nullptr;
        dbus_message_iter_get_basic(&args, &p);
        if (p) {
            start_request_path = p;
        }
    }

    if (start_request_path.empty()) {
        LogError << "Start: missing request path";
        dbus_message_unref(reply);
        return false;
    }

    // Add match rule so the D-Bus daemon delivers the Response signal
    DBusError err;
    dbus_error_init(&err);
    std::string match_rule = "type='signal',interface='" + std::string(kPortalRequestInterface) + "',path='" + start_request_path + "'";
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
                if (dbus_message_is_signal(sig_msg, kPortalRequestInterface, "Response")
                    && start_request_path == dbus_message_get_path(sig_msg)) {
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

                            if (key && strcmp(key, "streams") == 0 && dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_ARRAY) {
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
                            if (persist_) {
                                if (key && strcmp(key, "restore_token") == 0
                                    && dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_STRING) {
                                    void* restore_token = nullptr;
                                    dbus_message_iter_get_basic(&value, &restore_token);
                                    if (restore_token != nullptr) {
                                        restore_token_ = std::string(static_cast<char*>(restore_token));
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

void MaaToolkitPortalHelper::close_internal()
{
    pipewire_node_id_ = 0;
    pipewire_fd_ = -1;
    dbus_session_handle_.clear();
    if (dbus_connection_) {
        dbus_connection_unref(dbus_connection_);
        dbus_connection_ = nullptr;
    }
}

int MaaToolkitPortalHelper::get_fd() const
{
    return pipewire_fd_;
}

uint32_t MaaToolkitPortalHelper::get_node_id() const
{
    return pipewire_node_id_;
}

void MaaToolkitPortalHelper::set_persist(bool enable)
{
    persist_ = enable;
}

const char* MaaToolkitPortalHelper::get_restore_token() const
{
    return restore_token_.c_str();
}

void MaaToolkitPortalHelper::set_restore_token(const char* token)
{
    restore_token_ = std::string(token);
}

bool MaaToolkitPortalHelper::dbus_open_pipewire_remote(DBusConnection* conn)
{
    // OpenPipeWireRemote(session_handle, a{sv}) -> (h)
    DBusMessage* msg = dbus_message_new_method_call(kPortalBusName, kPortalPath, kPortalInterface, "OpenPipeWireRemote");
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
    if (!dbus_message_get_args(reply, &err, DBUS_TYPE_UNIX_FD, &raw_fd, DBUS_TYPE_INVALID)) {
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

#endif
