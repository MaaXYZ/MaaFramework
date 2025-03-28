/**
 * @file MaaTasker.h
 * @author
 * @brief The tasker API.
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

    MAA_FRAMEWORK_API MaaTasker* MaaTaskerCreate(MaaNotificationCallback notify, void* notify_trans_arg);

    MAA_FRAMEWORK_API void MaaTaskerDestroy(MaaTasker* tasker);

    /**
     * @param[in] value
     */
    MAA_FRAMEWORK_API MaaBool MaaTaskerSetOption(
        MaaTasker* tasker,
        MaaTaskerOption key,
        MaaOptionValue value /**< byte array, int*, char*, bool* */,
        MaaOptionValueSize val_size);

    MAA_FRAMEWORK_API MaaBool MaaTaskerBindResource(MaaTasker* tasker, MaaResource* res);

    MAA_FRAMEWORK_API MaaBool MaaTaskerBindController(MaaTasker* tasker, MaaController* ctrl);

    MAA_FRAMEWORK_API MaaBool MaaTaskerInited(const MaaTasker* tasker);

    MAA_FRAMEWORK_API MaaTaskId MaaTaskerPostTask(MaaTasker* tasker, const char* entry, const char* pipeline_override);

    MAA_FRAMEWORK_API MaaStatus MaaTaskerStatus(const MaaTasker* tasker, MaaTaskId id);

    MAA_FRAMEWORK_API MaaStatus MaaTaskerWait(const MaaTasker* tasker, MaaTaskId id);

    MAA_FRAMEWORK_API MaaBool MaaTaskerRunning(const MaaTasker* tasker);

    MAA_FRAMEWORK_API MaaTaskId MaaTaskerPostStop(MaaTasker* tasker);
    MAA_FRAMEWORK_API MaaBool MaaTaskerStopping(const MaaTasker* tasker);

    MAA_FRAMEWORK_API MaaResource* MaaTaskerGetResource(const MaaTasker* tasker);

    MAA_FRAMEWORK_API MaaController* MaaTaskerGetController(const MaaTasker* tasker);

    MAA_FRAMEWORK_API MaaBool MaaTaskerClearCache(MaaTasker* tasker);

    /**
     * @param[out] hit
     */
    MAA_FRAMEWORK_API MaaBool MaaTaskerGetRecognitionDetail(
        const MaaTasker* tasker,
        MaaRecoId reco_id,
        /* out */ MaaStringBuffer* node_name,
        /* out */ MaaStringBuffer* algorithm,
        /* out */ MaaBool* hit,
        /* out */ MaaRect* box,
        /* out */ MaaStringBuffer* detail_json,
        /* out */ MaaImageBuffer* raw,      // only valid in debug mode
        /* out */ MaaImageListBuffer* draws // only valid in debug mode
    );

    /**
     * @param[out] reco_id
     * @param[out] completed
     */
    MAA_FRAMEWORK_API MaaBool MaaTaskerGetNodeDetail(
        const MaaTasker* tasker,
        MaaNodeId node_id,
        /* out */ MaaStringBuffer* node_name,
        /* out */ MaaRecoId* reco_id,
        /* out */ MaaBool* completed);

    /**
     * @param[out] node_id_list
     * @param[in, out] node_id_list_size
     * @param[out] status
     */
    MAA_FRAMEWORK_API MaaBool MaaTaskerGetTaskDetail(
        const MaaTasker* tasker,
        MaaTaskId task_id,
        /* out */ MaaStringBuffer* entry,
        /* out */ MaaNodeId* node_id_list /**< array */,
        /* in & out */ MaaSize* node_id_list_size,
        /* out */ MaaStatus* status);

    /**
     * @param[out] latest_id
     */
    MAA_FRAMEWORK_API MaaBool MaaTaskerGetLatestNode(
        const MaaTasker* tasker,
        const char* node_name,
        /* out */ MaaNodeId* latest_id);

#ifdef __cplusplus
}
#endif
