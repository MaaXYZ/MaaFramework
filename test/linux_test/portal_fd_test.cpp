#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>
#include <unordered_set>

#include <dbus/dbus.h>
#include <gio/gio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "MaaToolkit/Portal/MaaToolkitPortal.h"

namespace
{
constexpr const char* kBusName = "org.freedesktop.portal.Desktop";
constexpr const char* kPortalPath = "/org/freedesktop/portal/desktop";
constexpr const char* kScreenCast = "org.freedesktop.portal.ScreenCast";
constexpr const char* kSession = "org.freedesktop.portal.Session";

void require(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(1);
    }
}

size_t count_fds()
{
    size_t count = 0;
    for ([[maybe_unused]] const auto& entry : std::filesystem::directory_iterator("/proc/self/fd")) {
        ++count;
    }
    return count;
}

enum class Fault
{
    None,
    CreateError,
    CreateCancel,
    CreateTimeout,
    SelectError,
    SelectCancel,
    SelectTimeout,
    StartError,
    StartCancel,
    StartTimeout,
    MissingNode,
    RemoteError,
    RemoteMissingFd,
    CurrentClosedSelect,
    CurrentClosedStart,
    ForeignClosedSelect,
    ForeignClosedStart,
};

class MockPortal
{
public:
    explicit MockPortal(bool fixed_session_path = false)
        : fixed_session_path_(fixed_session_path)
    {
        server_ = dbus_bus_get_private(DBUS_BUS_SESSION, nullptr);
        require(server_ != nullptr, "private bus connection failed");
        require(
            dbus_bus_request_name(server_, kBusName, DBUS_NAME_FLAG_DO_NOT_QUEUE, nullptr) == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER,
            "mock Portal name is unavailable");
        client_ = dbus_bus_get(DBUS_BUS_SESSION, nullptr);
        require(client_ != nullptr, "shared client connection failed");
        service_ = std::thread(&MockPortal::run, this);
        sync();
    }

    ~MockPortal()
    {
        sync();
        running_ = false;
        service_.join();
        dbus_connection_close(server_);
        dbus_connection_unref(server_);
        dbus_connection_unref(client_);
    }

    void set_fault(Fault fault) { fault_ = fault; }
    unsigned close_calls() const { return close_calls_; }
    unsigned live_sessions() const { return live_sessions_; }
    unsigned create_calls() const { return create_calls_; }
    unsigned request_close_calls() const { return request_close_calls_; }

    void sync()
    {
        // Use the same client connection as the helper so this reply is a
        // barrier for its preceding asynchronous Session.Close messages.
        auto* call = dbus_message_new_method_call(kBusName, kPortalPath, "org.maaframework.Test", "Barrier");
        auto* reply = dbus_connection_send_with_reply_and_block(client_, call, 5000, nullptr);
        dbus_message_unref(call);
        require(reply != nullptr, "mock Portal barrier failed");
        dbus_message_unref(reply);
    }

private:
    void send(DBusMessage* message)
    {
        require(dbus_connection_send(server_, message, nullptr), "mock Portal send failed");
        dbus_message_unref(message);
        dbus_connection_flush(server_);
    }

    void run()
    {
        while (running_) {
            dbus_connection_read_write(server_, 10);
            while (auto* message = dbus_connection_pop_message(server_)) {
                if (dbus_message_get_type(message) == DBUS_MESSAGE_TYPE_METHOD_CALL) {
                    respond(message);
                }
                dbus_message_unref(message);
            }
        }
    }

    void append_session(DBusMessageIter& dict, const std::string& session)
    {
        DBusMessageIter entry, value;
        const char* key = "session_handle";
        const char* path = session.c_str();
        dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
        dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
        dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "s", &value);
        dbus_message_iter_append_basic(&value, DBUS_TYPE_STRING, &path);
        dbus_message_iter_close_container(&entry, &value);
        dbus_message_iter_close_container(&dict, &entry);
    }

    void append_stream(DBusMessageIter& dict)
    {
        DBusMessageIter entry, value, streams, stream, properties;
        const char* key = "streams";
        uint32_t node_id = 42;
        dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
        dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
        dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "a(ua{sv})", &value);
        dbus_message_iter_open_container(&value, DBUS_TYPE_ARRAY, "(ua{sv})", &streams);
        dbus_message_iter_open_container(&streams, DBUS_TYPE_STRUCT, nullptr, &stream);
        dbus_message_iter_append_basic(&stream, DBUS_TYPE_UINT32, &node_id);
        dbus_message_iter_open_container(&stream, DBUS_TYPE_ARRAY, "{sv}", &properties);
        dbus_message_iter_close_container(&stream, &properties);
        dbus_message_iter_close_container(&streams, &stream);
        dbus_message_iter_close_container(&value, &streams);
        dbus_message_iter_close_container(&entry, &value);
        dbus_message_iter_close_container(&dict, &entry);
    }

    void send_closed(const char* session, const char* destination)
    {
        auto* closed = dbus_message_new_signal(session, kSession, "Closed");
        dbus_message_set_destination(closed, destination);
        DBusMessageIter args, details;
        dbus_message_iter_init_append(closed, &args);
        dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &details);
        dbus_message_iter_close_container(&args, &details);
        send(closed);
    }

    void respond(DBusMessage* call)
    {
        if (dbus_message_is_method_call(call, "org.maaframework.Test", "Barrier")) {
            send(dbus_message_new_method_return(call));
            return;
        }
        if (dbus_message_is_method_call(call, kSession, "Close")) {
            ++close_calls_;
            const char* session = dbus_message_get_path(call);
            const bool was_live = sessions_.erase(session) == 1;
            // A Portal-initiated Closed removes the remote object before the
            // helper's best-effort cleanup. Consume that history only once so
            // a duplicate Session.Close still fails the test.
            const bool closed_by_portal = closed_by_portal_.erase(session) == 1;
            require(was_live != closed_by_portal, "Session.Close used an unknown or already closed session");
            live_sessions_ = sessions_.size();
            require(dbus_message_get_no_reply(call), "Session.Close must not wait for a reply during cleanup");
            if (closed_by_portal) {
                return;
            }
            // A queued Closed signal from a retired session must not cancel
            // the next CreateSession/SelectSources/Start operation.
            send_closed(dbus_message_get_path(call), dbus_message_get_sender(call));
            return;
        }
        if (dbus_message_is_method_call(call, "org.freedesktop.portal.Request", "Close")) {
            ++request_close_calls_;
            require(pending_requests_.erase(dbus_message_get_path(call)) == 1, "Request.Close used an inactive request");
            require(dbus_message_get_no_reply(call), "Request.Close must not wait for a reply during cleanup");
            return;
        }
        require(dbus_message_has_interface(call, kScreenCast), "unexpected mock Portal interface");
        const char* method = dbus_message_get_member(call);
        const bool create = std::strcmp(method, "CreateSession") == 0;
        const bool select = std::strcmp(method, "SelectSources") == 0;
        const bool start = std::strcmp(method, "Start") == 0;
        const bool remote = std::strcmp(method, "OpenPipeWireRemote") == 0;
        require(create || select || start || remote, "unexpected mock Portal method");
        const Fault fault = fault_;
        const char* current_session = nullptr;
        if (create) {
            ++create_calls_;
        }
        else {
            DBusMessageIter args;
            dbus_message_iter_init(call, &args);
            require(dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_OBJECT_PATH, "missing session argument");
            dbus_message_iter_get_basic(&args, &current_session);
            require(sessions_.contains(current_session), "request used a closed or unknown session");
        }
        if ((create && fault == Fault::CreateError) || (select && fault == Fault::SelectError)
            || (start && fault == Fault::StartError) || (remote && fault == Fault::RemoteError)) {
            send(dbus_message_new_error(call, DBUS_ERROR_FAILED, "Injected Portal failure"));
            return;
        }
        auto* reply = dbus_message_new_method_return(call);
        if (remote) {
            if (fault == Fault::RemoteMissingFd) {
                send(reply);
                return;
            }
            int sockets[2];
            require(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets) == 0, "socketpair failed");
            dbus_message_append_args(reply, DBUS_TYPE_UNIX_FD, &sockets[0], DBUS_TYPE_INVALID);
            send(reply);
            close(sockets[0]);
            close(sockets[1]);
            return;
        }

        const std::string request = std::string(kPortalPath) + "/request/mock/request" + std::to_string(++request_id_);
        const char* request_path = request.c_str();
        dbus_message_append_args(reply, DBUS_TYPE_OBJECT_PATH, &request_path, DBUS_TYPE_INVALID);
        send(reply);
        if ((select && fault == Fault::CurrentClosedSelect) || (start && fault == Fault::CurrentClosedStart)) {
            // Return the request path first, then close the current session
            // without a Response: the helper must cancel the pending request.
            require(pending_requests_.insert(request).second, "duplicate pending request");
            require(sessions_.erase(current_session) == 1, "Portal closed an inactive session");
            live_sessions_ = sessions_.size();
            require(closed_by_portal_.insert(current_session).second, "Portal closed the same session twice");
            send_closed(current_session, dbus_message_get_sender(call));
            return;
        }
        if ((create && fault == Fault::CreateTimeout) || (select && fault == Fault::SelectTimeout)
            || (start && fault == Fault::StartTimeout)) {
            pending_requests_.insert(request);
            return;
        }
        if ((select && fault == Fault::ForeignClosedSelect) || (start && fault == Fault::ForeignClosedStart)) {
            send_closed("/org/freedesktop/portal/desktop/session/mock/foreign", dbus_message_get_sender(call));
        }
        auto* signal = dbus_message_new_signal(request_path, "org.freedesktop.portal.Request", "Response");
        dbus_message_set_destination(signal, dbus_message_get_sender(call));
        DBusMessageIter args, dict;
        dbus_message_iter_init_append(signal, &args);
        uint32_t response = (create && fault == Fault::CreateCancel) || (select && fault == Fault::SelectCancel)
                || (start && fault == Fault::StartCancel)
            ? 1
            : 0;
        dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &response);
        dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &dict);
        if (create && response == 0) {
            // Opt in to the reused path implied by the helper's fixed
            // session_handle_token. Other tests use unique paths to isolate
            // FD ownership from collisions between retained exported sessions.
            const std::string session = std::string(kPortalPath) + "/session/mock/"
                + (fixed_session_path_ ? "maa_session" : "session" + std::to_string(create_calls_));
            require(sessions_.insert(session).second, "duplicate mock session");
            live_sessions_ = sessions_.size();
            append_session(dict, session);
        }
        if (start && response == 0 && fault != Fault::MissingNode) {
            append_stream(dict);
        }
        dbus_message_iter_close_container(&args, &dict);
        send(signal);
    }

    const bool fixed_session_path_;
    DBusConnection* server_ = nullptr;
    DBusConnection* client_ = nullptr;
    std::atomic<bool> running_ = true;
    std::thread service_;
    std::atomic<Fault> fault_ = Fault::None;
    std::atomic<unsigned> close_calls_ = 0;
    std::atomic<unsigned> live_sessions_ = 0;
    std::atomic<unsigned> create_calls_ = 0;
    std::atomic<unsigned> request_close_calls_ = 0;
    unsigned request_id_ = 0;
    std::unordered_set<std::string> sessions_;
    std::unordered_set<std::string> closed_by_portal_;
    std::unordered_set<std::string> pending_requests_;
};

void open_and_release()
{
    auto* helper = MaaToolkitPortalHelperCreate();
    require(helper != nullptr, "helper creation failed");
    require(MaaToolkitPortalHelperOpenStream(helper), "mock Portal request failed");
    require(MaaToolkitPortalHelperGetPipeWireNodeID(helper) == 42, "incorrect node id");
    const int fd = MaaToolkitPortalHelperGetPipeWireFD(helper);
    require(fd >= 0, "missing remote fd");
    require(MaaToolkitPortalHelperGetPipeWireFD(helper) == fd, "repeated getter changed the caller-owned FD");
    // Existing callers destroy the helper before connecting PipeWire, which
    // consumes the exported FD. Destruction must preserve that contract.
    MaaToolkitPortalHelperDestroy(helper);
    require(close(fd) == 0, "caller could not release remote fd");
}

void test_fd_ownership()
{
    MockPortal portal;
    open_and_release();
    portal.sync();
    const size_t before = count_fds();
    for (int i = 0; i < 8; ++i) {
        open_and_release();
    }
    portal.sync();
    const size_t after = count_fds();
    std::printf("FDs before=%zu after=%zu\n", before, after);
    require(before == after, "OpenPipeWireRemote leaked file descriptors");
    require(portal.close_calls() == 0, "destroying the helper prematurely closed an exported session");
}

struct FailureCase
{
    const char* name;
    Fault fault;
    bool creates_session;
};

void test_failure(gconstpointer data)
{
    const auto& scenario = *static_cast<const FailureCase*>(data);
    MockPortal portal;
    const size_t before = count_fds();
    auto* helper = MaaToolkitPortalHelperCreate();
    require(helper != nullptr, "helper creation failed");
    portal.set_fault(scenario.fault);
    require(!MaaToolkitPortalHelperOpenStream(helper), "injected failure unexpectedly succeeded");
    require(MaaToolkitPortalHelperGetPipeWireFD(helper) == -1, "failed helper retained an FD");
    require(MaaToolkitPortalHelperGetPipeWireNodeID(helper) == 0, "failed helper retained a node id");
    portal.sync();
    const unsigned expected = scenario.creates_session ? 1 : 0;
    std::printf("%s: closes=%u live=%u\n", scenario.name, portal.close_calls(), portal.live_sessions());
    require(portal.close_calls() == expected, "failed Portal request did not close its session exactly once");
    require(portal.live_sessions() == 0, "failed Portal request left a live session");
    const bool timed_out = scenario.fault == Fault::CreateTimeout || scenario.fault == Fault::SelectTimeout
        || scenario.fault == Fault::StartTimeout;
    require(portal.request_close_calls() == (timed_out ? 1u : 0u), "timed-out Portal request was not closed exactly once");

    portal.set_fault(Fault::None);
    require(MaaToolkitPortalHelperOpenStream(helper), "same helper could not retry after failure");
    MaaToolkitPortalHelperDestroy(helper);
    portal.sync();
    require(portal.close_calls() == expected + 1, "retry/destruction did not close each owned session exactly once");
    require(portal.live_sessions() == 0, "retry/destruction left a live session");
    require(count_fds() == before, "failed request/retry/destruction leaked an FD");
}

void test_unclaimed_destruction()
{
    MockPortal portal;
    const size_t before = count_fds();
    for (int i = 0; i < 8; ++i) {
        auto* helper = MaaToolkitPortalHelperCreate();
        require(MaaToolkitPortalHelperOpenStream(helper), "mock Portal request failed");
        MaaToolkitPortalHelperDestroy(helper);
    }
    portal.sync();
    require(portal.close_calls() == 8, "unclaimed sessions were not closed on destruction");
    require(portal.live_sessions() == 0, "unclaimed session survived destruction");
    require(count_fds() == before, "unclaimed FD survived destruction");
}

void test_unclaimed_reopen()
{
    MockPortal portal(/*fixed_session_path=*/true);
    const size_t before = count_fds();
    auto* helper = MaaToolkitPortalHelperCreate();
    for (unsigned i = 0; i < 8; ++i) {
        require(MaaToolkitPortalHelperOpenStream(helper), "unclaimed helper could not reopen");
        portal.sync();
        require(portal.close_calls() == i, "reopening did not close the previous owned session");
        require(portal.live_sessions() == 1, "reopening left multiple live sessions");
    }
    MaaToolkitPortalHelperDestroy(helper);
    portal.sync();
    require(portal.close_calls() == 8, "reopen/destruction closed an owned session more or less than once");
    require(portal.live_sessions() == 0, "reopen/destruction left a live session");
    require(count_fds() == before, "reopen/destruction leaked an unclaimed FD");
}

void test_exported_reopen()
{
    MockPortal portal;
    const size_t before = count_fds();
    auto* helper = MaaToolkitPortalHelperCreate();
    require(MaaToolkitPortalHelperOpenStream(helper), "mock Portal request failed");
    const int fd = MaaToolkitPortalHelperGetPipeWireFD(helper);
    require(fd >= 0, "missing exported FD");
    require(!MaaToolkitPortalHelperOpenStream(helper), "reopening an exported helper unexpectedly succeeded");
    require(MaaToolkitPortalHelperGetPipeWireFD(helper) == fd, "reopening overwrote the exported FD");
    require(MaaToolkitPortalHelperGetPipeWireNodeID(helper) == 42, "reopening reset the active node id");
    MaaToolkitPortalHelperDestroy(helper);
    portal.sync();
    require(portal.create_calls() == 1, "reopening an exported helper created another session");
    require(portal.close_calls() == 0 && portal.live_sessions() == 1, "reopening/destruction closed an exported session");
    require(close(fd) == 0, "reopening/destruction invalidated the exported FD");
    require(count_fds() == before, "reopening/destruction leaked an FD");
}

void test_foreign_closed(gconstpointer data)
{
    MockPortal portal;
    portal.set_fault(*static_cast<const Fault*>(data));
    const size_t before = count_fds();
    auto* helper = MaaToolkitPortalHelperCreate();
    require(MaaToolkitPortalHelperOpenStream(helper), "an unrelated Closed signal cancelled the current session");
    MaaToolkitPortalHelperDestroy(helper);
    portal.sync();
    require(portal.close_calls() == 1 && portal.live_sessions() == 0, "current session was not cleaned up");
    require(count_fds() == before, "unrelated Closed signal handling leaked an FD");
}

void test_current_closed(gconstpointer data)
{
    MockPortal portal;
    portal.set_fault(*static_cast<const Fault*>(data));
    const size_t before = count_fds();
    auto* helper = MaaToolkitPortalHelperCreate();
    require(helper != nullptr, "helper creation failed");
    const auto started = std::chrono::steady_clock::now();
    require(!MaaToolkitPortalHelperOpenStream(helper), "current session Closed unexpectedly succeeded");
    const auto elapsed = std::chrono::steady_clock::now() - started;
    require(elapsed < std::chrono::seconds(5), "current session Closed waited for the Response timeout");
    require(MaaToolkitPortalHelperGetPipeWireFD(helper) == -1, "closed helper retained an FD");
    require(MaaToolkitPortalHelperGetPipeWireNodeID(helper) == 0, "closed helper retained a node id");
    portal.sync();
    std::printf(
        "current Closed: elapsed=%lldms closes=%u requests=%u live=%u\n",
        static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()),
        portal.close_calls(),
        portal.request_close_calls(),
        portal.live_sessions());
    require(portal.request_close_calls() == 1, "current session Closed did not close its pending request exactly once");
    require(portal.close_calls() == 1, "current session Closed did not attempt Session.Close exactly once");
    require(portal.live_sessions() == 0, "Portal-closed session remained live");
    require(count_fds() == before, "current session Closed leaked an FD");

    portal.set_fault(Fault::None);
    require(MaaToolkitPortalHelperOpenStream(helper), "same helper could not retry after current session Closed");
    require(MaaToolkitPortalHelperGetPipeWireNodeID(helper) == 42, "retry returned an incorrect node id");
    MaaToolkitPortalHelperDestroy(helper);
    portal.sync();
    require(portal.close_calls() == 2, "retry/destruction did not close the newly owned session exactly once");
    require(portal.request_close_calls() == 1, "retry/destruction closed an extra request");
    require(portal.live_sessions() == 0, "retry/destruction left a live session");
    require(count_fds() == before, "current session Closed/retry/destruction leaked an FD");
}
} // namespace

int main(int argc, char** argv)
{
    g_test_init(&argc, &argv, nullptr);
    require(dbus_threads_init_default(), "D-Bus threading initialization failed");
    GTestDBus* bus = g_test_dbus_new(G_TEST_DBUS_NONE);
    // Start our own daemon before any libdbus connection or service thread.
    g_test_dbus_up(bus);
    g_test_add_func("/portal/fd-ownership", test_fd_ownership);
    g_test_add_func("/portal/unclaimed-destruction", test_unclaimed_destruction);
    g_test_add_func("/portal/unclaimed-reopen", test_unclaimed_reopen);
    g_test_add_func("/portal/exported-reopen", test_exported_reopen);
    static constexpr Fault select_current_closed = Fault::CurrentClosedSelect;
    static constexpr Fault start_current_closed = Fault::CurrentClosedStart;
    g_test_add_data_func("/portal/current-closed/select", &select_current_closed, test_current_closed);
    g_test_add_data_func("/portal/current-closed/start", &start_current_closed, test_current_closed);
    static constexpr Fault select_foreign_closed = Fault::ForeignClosedSelect;
    static constexpr Fault start_foreign_closed = Fault::ForeignClosedStart;
    g_test_add_data_func("/portal/foreign-closed/select", &select_foreign_closed, test_foreign_closed);
    g_test_add_data_func("/portal/foreign-closed/start", &start_foreign_closed, test_foreign_closed);
    static constexpr FailureCase failures[] = {
        { "/portal/failure/create-error", Fault::CreateError, false },
        { "/portal/failure/create-cancel", Fault::CreateCancel, false },
        { "/portal/failure/create-timeout", Fault::CreateTimeout, false },
        { "/portal/failure/select-error", Fault::SelectError, true },
        { "/portal/failure/select-cancel", Fault::SelectCancel, true },
        { "/portal/failure/select-timeout", Fault::SelectTimeout, true },
        { "/portal/failure/start-error", Fault::StartError, true },
        { "/portal/failure/start-cancel", Fault::StartCancel, true },
        { "/portal/failure/start-timeout", Fault::StartTimeout, true },
        { "/portal/failure/missing-node", Fault::MissingNode, true },
        { "/portal/failure/remote-error", Fault::RemoteError, true },
        { "/portal/failure/remote-missing-fd", Fault::RemoteMissingFd, true },
    };
    for (const auto& failure : failures) {
        g_test_add_data_func(failure.name, &failure, test_failure);
    }
    const int result = g_test_run();
    g_test_dbus_down(bus);
    g_object_unref(bus);
    return result;
}
