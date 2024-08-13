/**
 * @file MaaContext.h
 * @author
 * @brief Context is a context for running tasks, recognizers, actions, and other operations.
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

    MAA_FRAMEWORK_API MaaTaskId MaaContextRunTask(MaaContext* context, const char* task_name, const char* param);

    MAA_FRAMEWORK_API MaaTaskId
        MaaContextRunRecognition(MaaContext* context, MaaImageBuffer* image, const char* task_name, const char* task_param);

    MAA_FRAMEWORK_API MaaTaskId MaaContextRunAction(
        MaaContext* context,
        const char* task_name,
        const char* task_param,
        MaaRect* cur_box,
        const char* cur_rec_detail);

    MAA_FRAMEWORK_API MaaTasker* MaaContextGetTasker(MaaContext* context);

#ifdef __cplusplus
}
#endif
