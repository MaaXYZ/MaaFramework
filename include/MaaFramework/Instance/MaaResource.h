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

    typedef MaaBool (*MaaCustomRecognizerCallback)(
        MaaContext* context,
        const char* task_name,
        const char* recognizer_name,
        const char* custom_recognition_param,
        const MaaImageBuffer* image,
        MaaTransparentArg trans_arg,
        /* out */ MaaRect* out_box,
        /* out */ MaaStringBuffer* out_detail);

    typedef MaaBool (*MaaCustomActionCallback)(
        MaaContext* context,
        const char* task_name,
        const char* action_name,
        const char* custom_action_param,
        const MaaRect* cur_box,
        const char* cur_rec_detail,
        MaaTransparentArg trans_arg);

    /**
     * @brief Create a resource.
     *
     * @param callback The callback function. See MaaNotificationCallback
     * @param callback_arg
     * @return MaaResource*
     */
    MAA_FRAMEWORK_API MaaResource* MaaResourceCreate(MaaNotificationCallback callback, MaaTransparentArg callback_arg);

    /**
     * @brief Free the resource.
     *
     * @param res
     */
    MAA_FRAMEWORK_API void MaaResourceDestroy(MaaResource* res);

    /**
     * @brief Register a custom recognizer to the instance.
     *
     * See MaaCustomRecognizer.h for details about how to create a custom recognizer.
     *
     * @param res
     * @param name The name of the recognizer that will be used to reference it.
     * @param recognizer
     * @param trans_arg
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaResourceRegisterCustomRecognizer(
        MaaResource* res,
        const char* name,
        MaaCustomRecognizerCallback recognizer,
        MaaTransparentArg trans_arg);

    /**
     * @brief Unregister a custom recognizer from the instance.
     *
     * @param res
     * @param name The name of the recognizer when it was registered.
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaResourceUnregisterCustomRecognizer(MaaResource* res, const char* name);

    /**
     * @brief Clear all custom recognizers registered to the instance.
     *
     * @param res
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaResourceClearCustomRecognizer(MaaResource* res);

    /**
     * @brief Register a custom action to the instance.
     *
     * See MaaCustomAction.h for details about how to create a custom action.
     *
     * @param res
     * @param name The name of the action that will be used to reference it.
     * @param action
     * @param trans_arg
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool
        MaaResourceRegisterCustomAction(MaaResource* res, const char* name, MaaCustomActionCallback action, MaaTransparentArg trans_arg);

    /**
     * @brief Unregister a custom action from the instance.
     *
     * @param res
     * @param name The name of the action when it was registered.
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaResourceUnregisterCustomAction(MaaResource* res, const char* name);

    /**
     * @brief Clear all custom actions registered to the instance.
     *
     * @param res
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaResourceClearCustomAction(MaaResource* res);

    /**
     * @brief Add a path to the resource loading paths
     *
     * @param res
     * @param path
     * @return MaaResId The id of the resource
     */
    MAA_FRAMEWORK_API MaaResId MaaResourcePostPath(MaaResource* res, const char* path);

    /**
     * @brief Clear the resource loading paths
     *
     * @param res
     * @return MaaBool Whether the resource is cleared successfully.
     */
    MAA_FRAMEWORK_API MaaBool MaaResourceClear(MaaResource* res);

    /**
     * @brief Get the loading status of a resource identified by id.
     *
     * @param res
     * @param id
     * @return MaaStatus
     */
    MAA_FRAMEWORK_API MaaStatus MaaResourceStatus(MaaResource* res, MaaResId id);

    /**
     * @brief Wait for the resource identified by id to finish loading.
     *
     * @param res
     * @param id
     * @return MaaStatus
     */
    MAA_FRAMEWORK_API MaaStatus MaaResourceWait(MaaResource* res, MaaResId id);

    /**
     * @brief Check if resources are loaded.
     *
     * @param res
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaResourceLoaded(MaaResource* res);

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
    MAA_FRAMEWORK_API MaaBool MaaResourceSetOption(MaaResource* res, MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size);

    /**
     * @brief Get the hash of the resource.
     *
     * @param res
     * @param buffer The buffer where the hash will be written to.
     *
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaResourceGetHash(MaaResource* res, /* out */ MaaStringBuffer* buffer);

    /**
     * @brief Get the task list of the resource.
     *
     * @param res
     * @param buffer The buffer where the task list will be written to.
     *
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaResourceGetTaskList(MaaResource* res, /* out */ MaaStringBuffer* buffer);

#ifdef __cplusplus
}
#endif
