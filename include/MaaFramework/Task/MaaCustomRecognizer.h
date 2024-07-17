/**
 * @file MaaCustomRecognizer.h
 * @author
 * @brief Custom recognizer API.
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
     * @brief The custom recognizer API.
     *
     * To create a custom recognizer, you need to implement this API.
     *
     * You do not have to implement all the functions in this API. Instead, just implement the
     * functions you need. Do note that if an unimplemented function is called, the framework will
     * likely crash.
     */
    struct MaaCustomRecognizerAPI
    {
        /// Write the recognition result to the out_box and return true if the recognition is
        /// successful. If the recognition fails, return false. You can also write details to the
        /// out_detail buffer.
        MaaBool (*analyze)(
            MaaSyncContextHandle sync_context,
            const MaaImageBufferHandle image,
            MaaStringView task_name,
            MaaStringView recognizer_name,
            MaaStringView custom_recognition_param,
            MaaTransparentArg recognizer_arg,
            /* out */ MaaRectHandle out_box,
            /* out */ MaaStringBufferHandle out_detail);
    };

#ifdef __cplusplus
}
#endif
