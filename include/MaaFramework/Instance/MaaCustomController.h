#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct MaaCustomControllerAPI
    {
        MaaBool (*connect)(MaaTransparentArg handle_arg);

        MaaBool (*request_uuid)(MaaTransparentArg handle_arg, /* out */ MaaStringBufferHandle buffer);
        MaaBool (*request_resolution)(MaaTransparentArg handle_arg, /* out */ int32_t* width,
                                      /* out */ int32_t* height);

        MaaBool (*start_app)(MaaStringView intent, MaaTransparentArg handle_arg);
        MaaBool (*stop_app)(MaaStringView intent, MaaTransparentArg handle_arg);

        MaaBool (*screencap)(MaaTransparentArg handle_arg, /* out */ MaaImageBufferHandle buffer);

        MaaBool (*click)(int32_t x, int32_t y, MaaTransparentArg handle_arg);
        MaaBool (*swipe)(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration,
                         MaaTransparentArg handle_arg);
        MaaBool (*touch_down)(int32_t contact, int32_t x, int32_t y, int32_t pressure, MaaTransparentArg handle_arg);
        MaaBool (*touch_move)(int32_t contact, int32_t x, int32_t y, int32_t pressure, MaaTransparentArg handle_arg);
        MaaBool (*touch_up)(int32_t contact, MaaTransparentArg handle_arg);

        MaaBool (*press_key)(int32_t keycode, MaaTransparentArg handle_arg);
        MaaBool (*input_text)(MaaStringView text, MaaTransparentArg handle_arg);
    };

#ifdef __cplusplus
}
#endif
