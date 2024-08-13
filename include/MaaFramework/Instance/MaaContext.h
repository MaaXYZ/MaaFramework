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

    MAA_FRAMEWORK_API MaaTaskId MaaContextRunPipeline(MaaContext* context, const char* task_name, const char* pipeline_override);

    MAA_FRAMEWORK_API MaaTaskId
        MaaContextRunRecognition(MaaContext* context, const char* task_name, const char* task_param, MaaImageBuffer* image);

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
