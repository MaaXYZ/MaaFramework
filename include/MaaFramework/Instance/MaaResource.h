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
     * @brief Register a custom recognizer to the instance.
     *
     * See MaaCustomRecognizer.h for details about how to create a custom recognizer.
     *
     * @param res
     * @param name The name of the recognizer that will be used to reference it.
     * @param recognizer
     * @param recognizer_arg
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaRegisterCustomRecognizer(
        MaaResourceHandle inst,
        MaaStringView name,
        MaaCustomRecognizerHandle recognizer,
        MaaTransparentArg recognizer_arg);

    /**
     * @brief Unregister a custom recognizer from the instance.
     *
     * @param res
     * @param name The name of the recognizer when it was registered.
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool
        MaaUnregisterCustomRecognizer(MaaResourceHandle inst, MaaStringView name);

    /**
     * @brief Clear all custom recognizers registered to the instance.
     *
     * @param res
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaClearCustomRecognizer(MaaResourceHandle inst);

    /**
     * @brief Register a custom action to the instance.
     *
     * See MaaCustomAction.h for details about how to create a custom action.
     *
     * @param res
     * @param name The name of the action that will be used to reference it.
     * @param action
     * @param action_arg
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaRegisterCustomAction(
        MaaResourceHandle inst,
        MaaStringView name,
        MaaCustomActionHandle action,
        MaaTransparentArg action_arg);

    /**
     * @brief Unregister a custom action from the instance.
     *
     * @param res
     * @param name The name of the action when it was registered.
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaUnregisterCustomAction(MaaResourceHandle inst, MaaStringView name);

    /**
     * @brief Clear all custom actions registered to the instance.
     *
     * @param res
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaClearCustomAction(MaaResourceHandle inst);

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
     * @param[in] value The option value.
     * @param val_size The size of the option value.
     * @return MaaBool Whether the option is set successfully.
     */
    MAA_FRAMEWORK_API MaaBool MaaResourceSetOption(
        MaaResourceHandle res,
        MaaResOption key,
        MaaOptionValue value /**< Maybe a byte array */,
        MaaOptionValueSize val_size);

    /**
     * @brief Get the hash of the resource.
     *
     * @param res
     * @param buffer The buffer where the hash will be written to.
     *
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool
        MaaResourceGetHash(MaaResourceHandle res, /* out */ MaaStringBufferHandle buffer);

    /**
     * @brief Get the task list of the resource.
     *
     * @param res
     * @param buffer The buffer where the task list will be written to.
     *
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool
        MaaResourceGetTaskList(MaaResourceHandle res, /* out */ MaaStringBufferHandle buffer);

#ifdef __cplusplus
}
#endif
