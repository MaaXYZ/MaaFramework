#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct MaaCustomControllerAPI
    {
        MaaBool (*set_option)(MaaCtrlOption key, MaaStringView value);

        MaaBool (*connect)(void);
        MaaBool (*click)(int32_t x, int32_t y);
        MaaBool (*swipe)(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration);

        MaaBool (*touch_down)(int32_t contact, int32_t x, int32_t y, int32_t pressure);
        MaaBool (*touch_move)(int32_t contact, int32_t x, int32_t y, int32_t pressure);
        MaaBool (*touch_up)(int32_t contact);

        MaaBool (*press_key)(int32_t keycode);

        MaaBool (*start_app)(MaaStringView package_name);
        MaaBool (*stop_app)(MaaStringView package_name);

        MaaBool (*get_resolution)(int32_t* width, int32_t* height);

        MaaBool (*get_image)(/* out */ MaaImageBufferHandle buffer);
        MaaBool (*get_uuid)(/* out */ MaaStringBufferHandle buffer);
    };

#ifdef __cplusplus
}
#endif
