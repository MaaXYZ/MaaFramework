#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct MaaCustomControllerAPI
    {
        MaaBool (*set_option)(MaaCtrlOption key, MaaStringView value, MaaTransparentArg arg);

        MaaBool (*connect)(MaaTransparentArg arg);
        MaaBool (*click)(int32_t x, int32_t y, MaaTransparentArg arg);
        MaaBool (*swipe)(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration, MaaTransparentArg arg);

        MaaBool (*touch_down)(int32_t contact, int32_t x, int32_t y, int32_t pressure, MaaTransparentArg arg);
        MaaBool (*touch_move)(int32_t contact, int32_t x, int32_t y, int32_t pressure, MaaTransparentArg arg);
        MaaBool (*touch_up)(int32_t contact, MaaTransparentArg arg);

        MaaBool (*press_key)(int32_t keycode, MaaTransparentArg arg);

        MaaBool (*start_app)(MaaStringView package_name, MaaTransparentArg arg);
        MaaBool (*stop_app)(MaaStringView package_name, MaaTransparentArg arg);

        MaaBool (*get_resolution)(int32_t* width, int32_t* height, MaaTransparentArg arg);

        MaaBool (*get_image)(/* out */ MaaImageBufferHandle buffer, MaaTransparentArg arg);
        MaaBool (*get_uuid)(/* out */ MaaStringBufferHandle buffer, MaaTransparentArg arg);
    };

#ifdef __cplusplus
}
#endif
