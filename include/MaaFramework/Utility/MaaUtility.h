/**
 * @file MaaUtility.h
 * @author
 * @brief Provide global functions for the framework.
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
     * @brief Get the version of the framework.
     *
     * @return MaaStringView
     */
    MAA_FRAMEWORK_API MaaStringView MaaVersion();

    /**
     * @brief Set options globally.
     *
     * This function requires a given set of option keys and value types, otherwise this will fail.
     * See #MaaGlobalOptionEnum for details.
     *
     * @param key The option key.
     * @param[in] value The option value.
     * @param val_size The size of the option value.
     * @return MaaBool Whether the option is set successfully.
     */
    MAA_FRAMEWORK_API MaaBool
        MaaSetGlobalOption(MaaGlobalOption key, MaaOptionValue value /**< Maybe a byte array */, MaaOptionValueSize val_size);

    /**
     * @param[out] hit
     */
    MAA_FRAMEWORK_API MaaBool MaaQueryRecognitionDetail(
        MaaRecoId reco_id,
        /* out */ MaaStringBufferHandle name,
        /* out */ MaaBool* hit,
        /* out */ MaaRectHandle hit_box,
        /* out */ MaaStringBufferHandle detail_json,
        /* out */ MaaImageBufferHandle raw,
        /* out */ MaaImageListBufferHandle draws);

    /**
     * @param[out] reco_id
     * @param[out] run_completed
     */
    MAA_FRAMEWORK_API MaaBool MaaQueryNodeDetail(
        MaaNodeId node_id,
        /* out */ MaaStringBufferHandle name,
        /* out */ MaaRecoId* reco_id,
        /* out */ MaaBool* run_completed);

    /**
     * @param[out] node_id_list
     * @param[in, out] node_id_list_size
     */
    MAA_FRAMEWORK_API MaaBool MaaQueryTaskDetail(
        MaaTaskId task_id,
        /* out */ MaaStringBufferHandle entry,
        /* out */ MaaNodeId* node_id_list /**< array */,
        /* in & out */ MaaSize* node_id_list_size);

#ifdef __cplusplus
}
#endif
