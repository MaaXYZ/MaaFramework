#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <thread>

#include <dbus/dbus.h>
#include <gio/gio.h>
#include <sys/socket.h>
#include <unistd.h>

#include "MaaToolkit/Portal/MaaToolkitPortal.h"

namespace
{
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

void respond(DBusConnection* connection, DBusMessage* call)
{
    const char* method = dbus_message_get_member(call);
    DBusMessage* reply = dbus_message_new_method_return(call);
    const char* request = "/org/freedesktop/portal/desktop/request/mock/request";
    const char* session = "/org/freedesktop/portal/desktop/session/mock/session";

    if (std::strcmp(method, "OpenPipeWireRemote") == 0) {
        int sockets[2];
        require(socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, sockets) == 0, "socketpair failed");
        dbus_message_append_args(reply, DBUS_TYPE_UNIX_FD, &sockets[0], DBUS_TYPE_INVALID);
        dbus_connection_send(connection, reply, nullptr);
        close(sockets[0]);
        close(sockets[1]);
    }
    else {
        dbus_message_append_args(reply, DBUS_TYPE_OBJECT_PATH, &request, DBUS_TYPE_INVALID);
        dbus_connection_send(connection, reply, nullptr);

        DBusMessage* signal = dbus_message_new_signal(request, "org.freedesktop.portal.Request", "Response");
        dbus_message_set_destination(signal, dbus_message_get_sender(call));
        DBusMessageIter args, dict;
        dbus_message_iter_init_append(signal, &args);
        uint32_t response = 0;
        dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &response);
        dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &dict);
        if (std::strcmp(method, "CreateSession") == 0) {
            DBusMessageIter entry, value;
            const char* key = "session_handle";
            dbus_message_iter_open_container(&dict, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
            dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);
            dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "s", &value);
            dbus_message_iter_append_basic(&value, DBUS_TYPE_STRING, &session);
            dbus_message_iter_close_container(&entry, &value);
            dbus_message_iter_close_container(&dict, &entry);
        }
        else if (std::strcmp(method, "Start") == 0) {
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
        dbus_message_iter_close_container(&args, &dict);
        dbus_connection_send(connection, signal, nullptr);
        dbus_message_unref(signal);
    }
    dbus_message_unref(reply);
    dbus_connection_flush(connection);
}

void open_and_release()
{
    auto* helper = MaaToolkitPortalHelperCreate();
    require(helper != nullptr, "helper creation failed");
    require(MaaToolkitPortalHelperOpenStream(helper), "mock Portal request failed");
    require(MaaToolkitPortalHelperGetPipeWireNodeID(helper) == 42, "incorrect node id");
    const int fd = MaaToolkitPortalHelperGetPipeWireFD(helper);
    require(fd >= 0, "missing remote fd");
    // The FD is handed to the caller, just as when connecting a PipeWire core.
    MaaToolkitPortalHelperDestroy(helper);
    require(close(fd) == 0, "caller could not release remote fd");
}

void test_fd_ownership()
{
    require(dbus_threads_init_default(), "D-Bus threading initialization failed");
    DBusConnection* server = dbus_bus_get_private(DBUS_BUS_SESSION, nullptr);
    require(server != nullptr, "private bus connection failed");
    require(
        dbus_bus_request_name(server, "org.freedesktop.portal.Desktop", DBUS_NAME_FLAG_DO_NOT_QUEUE, nullptr)
            == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER,
        "mock Portal name is unavailable");

    std::atomic<bool> running = true;
    std::thread service([&]() {
        while (running) {
            dbus_connection_read_write(server, 10);
            while (auto* message = dbus_connection_pop_message(server)) {
                if (dbus_message_has_interface(message, "org.freedesktop.portal.ScreenCast")) {
                    respond(server, message);
                }
                dbus_message_unref(message);
            }
        }
    });

    open_and_release(); // Establish the shared client connection before counting.
    const size_t before = count_fds();
    for (int i = 0; i < 8; ++i) {
        open_and_release();
    }
    const size_t after = count_fds();
    running = false;
    service.join();
    dbus_connection_close(server);
    dbus_connection_unref(server);
    std::printf("FDs before=%zu after=%zu\n", before, after);
    require(before == after, "OpenPipeWireRemote leaked file descriptors");
}
} // namespace

int main(int argc, char** argv)
{
    g_test_init(&argc, &argv, nullptr);
    GTestDBus* bus = g_test_dbus_new(G_TEST_DBUS_NONE);
    // Start our own daemon before any libdbus connection or service thread.
    g_test_dbus_up(bus);
    g_test_add_func("/portal/fd-ownership", test_fd_ownership);
    const int result = g_test_run();
    g_test_dbus_down(bus);
    g_object_unref(bus);
    return result;
}
