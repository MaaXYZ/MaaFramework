/**
 * @file MaaResource.h
 * @author
 * @brief The resource API.
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
     * @brief Create a resource.
     *
     * @param callback The callback function. See MaaAPICallback
     * @param callback_arg
     * @return MaaResourceHandle
     */
    MAA_FRAMEWORK_API MaaResourceHandle
        MaaResourceCreate(MaaResourceCallback callback, MaaCallbackTransparentArg callback_arg);

    /**
     * @brief Free the resource.
     *
     * @param res
     */
    MAA_FRAMEWORK_API void MaaResourceDestroy(MaaResourceHandle res);

    /**
     * @brief Add a path to the resource loading paths
     *
     * @param res
     * @param path
     * @return MaaResId The id of the resource
     */
    MAA_FRAMEWORK_API MaaResId MaaResourcePostPath(MaaResourceHandle res, MaaStringView path);

    /**
     * @brief Clear the resource loading paths
     *
     * @param res
     * @return MaaBool Whether the resource is cleared successfully.
     */
    MAA_FRAMEWORK_API MaaBool MaaResourceClear(MaaResourceHandle res);

    /**
     * @brief Get the loading status of a resource identified by id.
     *
     * @param res
     * @param id
     * @return MaaStatus
     */
    MAA_FRAMEWORK_API MaaStatus MaaResourceStatus(MaaResourceHandle res, MaaResId id);

    /**
     * @brief Wait for a resource to be loaded.
     *
     * @param res
     * @param id
     * @return MaaStatus
     */
    MAA_FRAMEWORK_API MaaStatus MaaResourceWait(MaaResourceHandle res, MaaResId id);

    /**
     * @brief Check if resources are loaded.
     *
     * @param res
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaResourceLoaded(MaaResourceHandle res);

    /**
     * @brief Set options for a given resource.
     *
     * This function requires a given set of option keys and value types, otherwise this will fail.
     * See #MaaResOptionEnum for details.
     *
     * @param res The handle of the resource to set options for.
     * @param key The option key.
     * @param value The option value.
     * @param val_size The size of the option value.
     * @return MaaBool Whether the option is set successfully.
     */
    MAA_FRAMEWORK_API MaaBool MaaResourceSetOption(
        MaaResourceHandle res,
        MaaResOption key,
        MaaOptionValue value,
        MaaOptionValueSize val_size);

    /**
     * @brief Get the hash of the resource.
     *
     * @param res
     * @param[out] buffer The buffer where the hash will be written to.
     *
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool
        MaaResourceGetHash(MaaResourceHandle res, MaaStringBufferHandle buffer);

    /**
     * @brief Get the task list of the resource.
     *
     * @param res
     * @param[out] buffer The buffer where the task list will be written to.
     *
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool
        MaaResourceGetTaskList(MaaResourceHandle res, MaaStringBufferHandle buffer);

#ifdef __cplusplus
}
#endif
