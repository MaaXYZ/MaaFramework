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

    MAA_FRAMEWORK_API MaaActId MaaContextRunAction(
        MaaContext* context,
        const char* entry,
        const char* pipeline_override,
        const MaaRect* box,
        const char* reco_detail);

    /**
     * @brief Run recognition directly with type and parameters, without requiring a pipeline entry.
     *
     * @param reco_type Recognition type string (e.g., "OCR", "TemplateMatch")
     * @param reco_param Recognition parameters json
     * @param image Image to recognize
     */
    MAA_FRAMEWORK_API MaaRecoId
        MaaContextRunRecognitionDirect(MaaContext* context, const char* reco_type, const char* reco_param, const MaaImageBuffer* image);

    /**
     * @brief Run action directly with type and parameters, without requiring a pipeline entry.
     *
     * @param action_type Action type string (e.g., "Click", "Swipe")
     * @param action_param Action parameters json
     * @param box Previous recognition position
     * @param reco_detail Previous recognition details
     */
    MAA_FRAMEWORK_API MaaActId MaaContextRunActionDirect(
        MaaContext* context,
        const char* action_type,
        const char* action_param,
        const MaaRect* box,
        const char* reco_detail);

    MAA_FRAMEWORK_API MaaBool MaaContextWaitFreezes(MaaContext* context, MaaSize time, const MaaRect* roi, const char* wait_freezes_param);

    MAA_FRAMEWORK_API MaaBool MaaContextOverridePipeline(MaaContext* context, const char* pipeline_override);

    MAA_FRAMEWORK_API MaaBool MaaContextOverrideNext(MaaContext* context, const char* node_name, const MaaStringListBuffer* next_list);

    MAA_FRAMEWORK_API MaaBool MaaContextOverrideImage(MaaContext* context, const char* image_name, const MaaImageBuffer* image);

    MAA_FRAMEWORK_API MaaBool MaaContextGetNodeData(MaaContext* context, const char* node_name, /* out */ MaaStringBuffer* buffer);

    MAA_FRAMEWORK_API MaaTaskId MaaContextGetTaskId(const MaaContext* context);

    MAA_FRAMEWORK_API MaaTasker* MaaContextGetTasker(const MaaContext* context);

    MAA_FRAMEWORK_API MaaContext* MaaContextClone(const MaaContext* context);

    MAA_FRAMEWORK_API MaaBool MaaContextSetAnchor(MaaContext* context, const char* anchor_name, const char* node_name);
    MAA_FRAMEWORK_API MaaBool MaaContextGetAnchor(MaaContext* context, const char* anchor_name, /* out */ MaaStringBuffer* buffer);
    MAA_FRAMEWORK_API MaaBool MaaContextGetHitCount(MaaContext* context, const char* node_name, /* out */ MaaSize* count);
    MAA_FRAMEWORK_API MaaBool MaaContextClearHitCount(MaaContext* context, const char* node_name);

#ifdef __cplusplus
}
#endif
