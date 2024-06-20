/**
 * @file MaaCustomController.h
 * @author
 * @brief Custom controller API.
 *
 * @copyright Copyright (c) 2024
 *
 */

// IWYU pragma: private, include <MaaFramework/MaaAPI.h>

#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief The custom controller API.
     *
     * To create a custom controller, you need to implement this API.
     *
     * You do not have to implement all the functions in this API. Instead, just implement the
     * functions you need. Do note that if an unimplemented function is called, the framework will
     * likely crash.
     */
    struct MaaCustomControllerAPI
    {
        MaaBool (*connect)(MaaTransparentArg handle_arg);

        /// Write result to buffer.
        MaaBool (
            *request_uuid)(MaaTransparentArg handle_arg, /* out */ MaaStringBufferHandle buffer);

        MaaBool (*start_app)(MaaStringView intent, MaaTransparentArg handle_arg);
        MaaBool (*stop_app)(MaaStringView intent, MaaTransparentArg handle_arg);

        /// Write result to buffer.
        MaaBool (*screencap)(MaaTransparentArg handle_arg, /* out */ MaaImageBufferHandle buffer);

        MaaBool (*click)(int32_t x, int32_t y, MaaTransparentArg handle_arg);
        MaaBool (*swipe)(
            int32_t x1,
            int32_t y1,
            int32_t x2,
            int32_t y2,
            int32_t duration,
            MaaTransparentArg handle_arg);
        MaaBool (*touch_down)(
            int32_t contact,
            int32_t x,
            int32_t y,
            int32_t pressure,
            MaaTransparentArg handle_arg);
        MaaBool (*touch_move)(
            int32_t contact,
            int32_t x,
            int32_t y,
            int32_t pressure,
            MaaTransparentArg handle_arg);
        MaaBool (*touch_up)(int32_t contact, MaaTransparentArg handle_arg);

        MaaBool (*press_key)(int32_t keycode, MaaTransparentArg handle_arg);
        MaaBool (*input_text)(MaaStringView text, MaaTransparentArg handle_arg);
    };

#ifdef __cplusplus
}
#endif
