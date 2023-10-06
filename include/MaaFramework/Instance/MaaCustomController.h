#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct MaaCustomControllerAPI
    {
        MaaBool (*set_option)(MaaCtrlOption key, MaaStringView value, MaaTransparentArg handle_arg);

        MaaBool (*connect)(MaaTransparentArg handle_arg);
        MaaBool (*click)(int32_t x, int32_t y, MaaTransparentArg handle_arg);
        MaaBool (*swipe)(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration,
                         MaaTransparentArg handle_arg);
        MaaBool (*press_key)(int32_t keycode, MaaTransparentArg handle_arg);

        MaaBool (*touch_down)(int32_t contact, int32_t x, int32_t y, int32_t pressure, MaaTransparentArg handle_arg);
        MaaBool (*touch_move)(int32_t contact, int32_t x, int32_t y, int32_t pressure, MaaTransparentArg handle_arg);
        MaaBool (*touch_up)(int32_t contact, MaaTransparentArg handle_arg);

        MaaBool (*start_app)(MaaStringView package_name, MaaTransparentArg handle_arg);
        MaaBool (*stop_app)(MaaStringView package_name, MaaTransparentArg handle_arg);

        MaaBool (*get_resolution)(MaaTransparentArg handle_arg, /* out */ int32_t* width,
                                  /* out */ int32_t* height);

        MaaBool (*get_image)(MaaTransparentArg handle_arg, /* out */ MaaImageBufferHandle buffer);
        MaaBool (*get_uuid)(MaaTransparentArg handle_arg, /* out */ MaaStringBufferHandle buffer);
    };

#ifdef __cplusplus
}
#endif
