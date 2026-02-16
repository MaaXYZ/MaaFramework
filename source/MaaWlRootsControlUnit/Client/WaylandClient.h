#pragma once

#include "MemfdBuffer.h"
#include "WaylandHelper.h"

#include <filesystem>
#include <string>

#include "Common/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

inline uint64_t get_ms()
{
    timespec ts = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<uint64_t>(ts.tv_sec) * UINT64_C(1000) + static_cast<uint64_t>(ts.tv_nsec) / UINT64_C(1'000'000);
}

class WaylandClient
{
public:
    enum class EventPhase : uint8_t
    {
        Began = 0,
        Moved = 1,
        Ended = 3,
    };

    WaylandClient();
    ~WaylandClient();

    WaylandClient(const WaylandClient&) = delete;
    WaylandClient& operator=(const WaylandClient&) = delete;

    bool connect(const std::filesystem::path& socket_path);
    void close();
    bool connected() const;

    bool screencap(void** buffer, uint32_t& width, uint32_t& height, uint32_t& format);
    bool pointer(EventPhase phase, int x, int y, int contact);
    bool scroll(int dx, int dy);
    bool input(EventPhase phase, int key);
    std::pair<int, int> screen_size() const;

private:
    bool open();
    bool bind_protocol();
    bool prepare_device();
    bool process_requests() const;
    bool check_buffer(int format, int width, int height, int stride) const;
    bool create_buffer(int format, int width, int height, int stride);
    bool close_buffer();

    std::unique_ptr<wl_display> display_;
    std::unique_ptr<wl_registry> registry_;
    std::unique_ptr<wl_output> output_;
    std::unique_ptr<wl_shm> shm_;
    std::unique_ptr<wl_seat> seat_;
    std::unique_ptr<zwlr_screencopy_manager_v1> screencopy_manager_;
    std::unique_ptr<zwlr_virtual_pointer_manager_v1> pointer_manager_;
    std::unique_ptr<zwlr_virtual_pointer_v1> pointer_;
    std::unique_ptr<zwp_virtual_keyboard_manager_v1> keyboard_manager_;
    std::unique_ptr<zwp_virtual_keyboard_v1> keyboard_;

    std::pair<int, int> screen_size_ { 0, 0 };

    wl_display_listener display_listener_ = { };
    wl_registry_listener registry_listener_ = { };

    std::filesystem::path socket_path_;

    bool capture_waiting_ = false;
    bool capture_successful_ = false;

    std::unique_ptr<MemfdBuffer> buffer_;
    std::unique_ptr<wl_shm_pool> shm_pool_;
    std::unique_ptr<wl_buffer> buffer_obj_;
    int buffer_format_ = 0;
};

MAA_CTRL_UNIT_NS_END
