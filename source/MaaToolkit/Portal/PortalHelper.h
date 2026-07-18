#pragma once

#include <string>

struct DBusConnection;

struct MaaToolkitPortalHelper
{
public:
    bool dbus_create_session();
    bool dbus_select_sources();
    bool dbus_start_stream();
    void close_internal();

    int get_fd() const;
    uint32_t get_node_id() const;

private:
    bool dbus_open_pipewire_remote(DBusConnection* conn);

    DBusConnection* dbus_connection_ = nullptr;
    std::string dbus_session_handle_;
    uint32_t pipewire_node_id_ = 0;
    int pipewire_fd_ = -1;
};
