#pragma once
#include "Common/Conf.h"
#include "WaylandHelper.h"
#include "filesystem"

MAA_CTRL_UNIT_NS_BEGIN

class WaylandClient
{
public:
    WaylandClient();
    ~WaylandClient();

    WaylandClient(const WaylandClient&) = delete;
    WaylandClient& operator=(const WaylandClient&) = delete;

    bool connect(const std::filesystem::path& socket_path);
    bool connected();
    bool process_requests() const;
    void close();

    std::shared_ptr<zwlr_screencopy_manager_v1> get_screencopy_manager();
    std::shared_ptr<zwlr_virtual_pointer_manager_v1> get_virtual_pointer_manager();
    std::shared_ptr<zwp_virtual_keyboard_manager_v1> get_virtual_keyboard_manager();
    std::shared_ptr<wl_shm> get_shm();
    std::shared_ptr<wl_seat> get_seat();
    std::shared_ptr<wl_output> get_output();

private:
    bool open();
    bool bind_protocol();

private:
    std::filesystem::path socket_path_;
    bool connected_ = false;

    wl_registry_listener registry_listener_ = { };

    // Registry objects
    std::unique_ptr<wl_display> display_;
    std::unique_ptr<wl_registry> registry_;
    std::shared_ptr<wl_output> output_;
    std::shared_ptr<wl_shm> shm_;
    std::shared_ptr<wl_seat> seat_;
    std::shared_ptr<zwlr_screencopy_manager_v1> screencopy_manager_;
    std::shared_ptr<zwlr_virtual_pointer_manager_v1> pointer_manager_;
    std::shared_ptr<zwp_virtual_keyboard_manager_v1> keyboard_manager_;
};

MAA_CTRL_UNIT_NS_END
