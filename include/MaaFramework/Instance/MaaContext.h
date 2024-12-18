/**
 * @file MaaContext.h
 * @author
 * @brief Context is a context for running tasks, recognitions, actions, and other operations.
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

    MAA_FRAMEWORK_API MaaTaskId MaaContextRunTask(MaaContext* context, const char* entry, const char* pipeline_override);

    MAA_FRAMEWORK_API MaaRecoId
        MaaContextRunRecognition(MaaContext* context, const char* entry, const char* pipeline_override, const MaaImageBuffer* image);

    MAA_FRAMEWORK_API MaaNodeId MaaContextRunAction(
        MaaContext* context,
        const char* entry,
        const char* pipeline_override,
        const MaaRect* box,
        const char* reco_detail);

    MAA_FRAMEWORK_API MaaBool MaaContextOverridePipeline(MaaContext* context, const char* pipeline_override);

    MAA_FRAMEWORK_API MaaBool MaaContextOverrideNext(MaaContext* context, const char* node_name, const MaaStringListBuffer* next_list);

    MAA_FRAMEWORK_API MaaTaskId MaaContextGetTaskId(const MaaContext* context);

    MAA_FRAMEWORK_API MaaTasker* MaaContextGetTasker(const MaaContext* context);

    MAA_FRAMEWORK_API MaaContext* MaaContextClone(const MaaContext* context);

#ifdef __cplusplus
}
#endif
