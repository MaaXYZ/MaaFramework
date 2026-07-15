#include "WaylandClient.h"

#include "MaaUtils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

WaylandClient::WaylandClient()
{
}

WaylandClient::~WaylandClient()
{
    close();
}

bool WaylandClient::connect(const std::filesystem::path& socket_path)
{
    if (socket_path_ != socket_path) {
        LogInfo << "Closing old socket" << socket_path_;
        close();
        socket_path_ = socket_path;
    }

    return open();
}

bool WaylandClient::connected()
{
    return connected_;
}

bool WaylandClient::open()
{
    LogInfo << "Connecting to socket" << VAR(socket_path_);
    if (display_) {
        return true;
    }

    display_.reset(wl_display_connect(socket_path_.c_str()));
    if (!display_) {
        LogError << "Failed to connect wayland socket " << VAR(socket_path_);
        return false;
    }

    if (!bind_protocol()) {
        LogError << "Failed to bind protocols";
        close();
        return false;
    }
    connected_ = true;

    return true;
}

void WaylandClient::close()
{
    connected_ = false;
    LogInfo << "Closing the wayland socket";
}

bool WaylandClient::bind_protocol()
{
    registry_.reset(wl_display_get_registry(display_.get()));
    if (!registry_) {
        LogError << "Failed to get registry";
        return false;
    }

    registry_listener_.global = [](void* data, struct wl_registry* registry, uint32_t id, const char* interface, uint32_t version) {
        const auto self = static_cast<WaylandClient*>(data);

#define BIND(interface_type, global_member)                                                                                            \
    if (std::string_view(interface) == #interface_type) {                                                                              \
        LogDebug << "Bind protocol" << VAR(interface);                                                                                 \
        self->global_member.reset(static_cast<interface_type*>(wl_registry_bind(registry, id, &interface_type##_interface, version))); \
        return;                                                                                                                        \
    }

        BIND(wl_output, output_);
        BIND(wl_shm, shm_);
        BIND(wl_seat, seat_);
        BIND(zwlr_screencopy_manager_v1, screencopy_manager_);
        BIND(zwp_virtual_keyboard_manager_v1, keyboard_manager_);
        BIND(zwlr_virtual_pointer_manager_v1, pointer_manager_);

#undef BIND
    };
    registry_listener_.global_remove =
        []([[maybe_unused]] void* data, [[maybe_unused]] struct wl_registry* registry, [[maybe_unused]] uint32_t name) {
            // nop
        };
    wl_registry_add_listener(registry_.get(), &registry_listener_, this);
    if (!process_requests()) {
        return false;
    }
    if (!output_ && !shm_) {
        LogError << "Compositor doesn't support wayland core protocol";
        return false;
    }
    if (!screencopy_manager_) {
        LogError << "Compositor doesn't support wlr_screencopy_unstable_v1 protocol";
        return false;
    }
    if (!keyboard_manager_) {
        LogError << "Compositor doesn't support virtual_keyboard_unstable_v1 protocol";
        return false;
    }
    if (!pointer_manager_) {
        LogError << "Compositor doesn't support wlr_virtual_pointer_unstable_v1 protocol";
        return false;
    }

    return true;
}

bool WaylandClient::process_requests() const
{
    if (const int result = wl_display_roundtrip(display_.get()); result == -1) {
        LogError << "Failed to process requests";
        return false;
    }

    return true;
}

std::shared_ptr<zwlr_screencopy_manager_v1> WaylandClient::get_screencopy_manager()
{
    std::shared_ptr p(screencopy_manager_);
    return p;
}

std::shared_ptr<zwlr_virtual_pointer_manager_v1> WaylandClient::get_virtual_pointer_manager()
{
    std::shared_ptr p(pointer_manager_);
    return p;
}

std::shared_ptr<zwp_virtual_keyboard_manager_v1> WaylandClient::get_virtual_keyboard_manager()
{
    std::shared_ptr p(keyboard_manager_);
    return p;
}

std::shared_ptr<wl_shm> WaylandClient::get_shm()
{
    std::shared_ptr p(shm_);
    return p;
}

std::shared_ptr<wl_seat> WaylandClient::get_seat()
{
    std::shared_ptr p(seat_);
    return p;
}

std::shared_ptr<wl_output> WaylandClient::get_output()
{
    std::shared_ptr p(output_);
    return p;
}

MAA_CTRL_UNIT_NS_END
