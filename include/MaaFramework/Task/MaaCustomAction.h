/**
 * @file MaaCustomAction.h
 * @author
 * @brief Custom action API.
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
     * @brief The custom action API.
     *
     * To create a custom action, you need to implement this API.
     *
     * You do not have to implement all the functions in this API. Instead, just implement the
     * functions you need. Do note that if an unimplemented function is called, the framework will
     * likely crash.
     */
    struct MaaCustomActionAPI
    {
        MaaBool (*run)(
            MaaSyncContextHandle sync_context,
            const MaaImageBufferHandle image,
            MaaStringView task_name,
            MaaStringView action_name,
            MaaStringView custom_action_param,
            MaaRectHandle cur_box,
            MaaStringView cur_rec_detail,
            MaaTransparentArg action_arg);
    };

#ifdef __cplusplus
}
#endif
