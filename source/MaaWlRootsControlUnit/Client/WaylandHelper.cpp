#include "WaylandHelper.h"

#include "MaaUtils/Logger.h"

#define DEFAULT_DELETER(TypeName, DeleteFunction)            \
    namespace std                                            \
    {                                                        \
    void default_delete<TypeName>::operator()(TypeName* ptr) \
    {                                                        \
        LogDebug << "Delete protocol";                       \
        DeleteFunction(ptr);                                 \
    }                                                        \
    }

DEFAULT_DELETER(wl_display, wl_display_disconnect)
DEFAULT_DELETER(wl_registry, wl_registry_destroy)
DEFAULT_DELETER(wl_output, wl_output_destroy)
DEFAULT_DELETER(wl_shm, wl_shm_destroy)
DEFAULT_DELETER(wl_seat, wl_seat_destroy)
DEFAULT_DELETER(wl_shm_pool, wl_shm_pool_destroy)
DEFAULT_DELETER(wl_buffer, wl_buffer_destroy)
DEFAULT_DELETER(zwlr_screencopy_manager_v1, zwlr_screencopy_manager_v1_destroy)
DEFAULT_DELETER(zwlr_screencopy_frame_v1, zwlr_screencopy_frame_v1_destroy)
DEFAULT_DELETER(zwlr_virtual_pointer_manager_v1, zwlr_virtual_pointer_manager_v1_destroy)
DEFAULT_DELETER(zwlr_virtual_pointer_v1, zwlr_virtual_pointer_v1_destroy)
DEFAULT_DELETER(zwp_virtual_keyboard_manager_v1, zwp_virtual_keyboard_manager_v1_destroy)
DEFAULT_DELETER(zwp_virtual_keyboard_v1, zwp_virtual_keyboard_v1_destroy)

#undef DEFAULT_DELETER

MAA_CTRL_UNIT_NS_BEGIN
void WaylandHelper::randname(std::string& name)
{
    timespec ts = { };
    clock_gettime(CLOCK_REALTIME, &ts);
    long r = ts.tv_nsec;
    for (int i = 0; i < 6; ++i) {
        name[i] = 'A' + (r & 15) + (r & 16) * 2;
        r >>= 5;
    }
}

MAA_CTRL_UNIT_NS_END
