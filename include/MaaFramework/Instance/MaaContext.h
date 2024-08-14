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

    MAA_FRAMEWORK_API MaaTaskId MaaContextRunPipeline(MaaContext* context, const char* entry, const char* pipeline_override);

    MAA_FRAMEWORK_API MaaTaskId
        MaaContextRunRecognition(MaaContext* context, const char* entry, const char* pipeline_override, MaaImageBuffer* image);

    MAA_FRAMEWORK_API MaaTaskId
        MaaContextRunAction(MaaContext* context, const char* entry, const char* pipeline_override, MaaRect* box, const char* reco_detail);

    MAA_FRAMEWORK_API MaaBool MaaContextOverridePipeline(MaaContext* context, const char* pipeline_override);

    MAA_FRAMEWORK_API MaaTaskId MaaContextGetTaskId(MaaContext* context);

    MAA_FRAMEWORK_API MaaTasker* MaaContextGetTasker(MaaContext* context);

#ifdef __cplusplus
}
#endif
