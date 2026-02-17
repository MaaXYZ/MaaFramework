#pragma once
#include "Common/Conf.h"
#include "wayland/virtual-keyboard.h"
#include "wayland/wlr-screencopy.h"
#include "wayland/wlr-virtual-pointer.h"
#include <memory>
#include <wayland-client-protocol.h>
#define DEFAULT_DELETER_FDECL(TypeName) \
    namespace std                       \
    {                                   \
    template <>                         \
    struct default_delete<TypeName>     \
    {                                   \
        void operator()(TypeName* ptr); \
    };                                  \
    }

DEFAULT_DELETER_FDECL(wl_display)
DEFAULT_DELETER_FDECL(wl_registry)
DEFAULT_DELETER_FDECL(wl_output)
DEFAULT_DELETER_FDECL(wl_seat)
DEFAULT_DELETER_FDECL(wl_shm)
DEFAULT_DELETER_FDECL(wl_shm_pool)
DEFAULT_DELETER_FDECL(wl_buffer)
DEFAULT_DELETER_FDECL(zwlr_screencopy_manager_v1)
DEFAULT_DELETER_FDECL(zwlr_screencopy_frame_v1)
DEFAULT_DELETER_FDECL(zwlr_virtual_pointer_manager_v1)
DEFAULT_DELETER_FDECL(zwlr_virtual_pointer_v1)
DEFAULT_DELETER_FDECL(zwp_virtual_keyboard_manager_v1)
DEFAULT_DELETER_FDECL(zwp_virtual_keyboard_v1)
MAA_CTRL_UNIT_NS_BEGIN

class WaylandHelper
{
public:
    static uint64_t get_ms()
    {
        timespec ts = { 0 };
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return static_cast<uint64_t>(ts.tv_sec) * UINT64_C(1000) + static_cast<uint64_t>(ts.tv_nsec) / UINT64_C(1'000'000);
    }

    static void randname(std::string& name);
};

MAA_CTRL_UNIT_NS_END
