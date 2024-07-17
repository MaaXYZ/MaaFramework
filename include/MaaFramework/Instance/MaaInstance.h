/**
 * @file MaaInstance.h
 * @author
 * @brief The instance API.
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
     * @brief Create an instance.
     *
     * @param callback The callback function. See MaaAPICallback
     * @param callback_arg The callback arg that will be passed to the callback function.
     * @return MaaInstanceHandle
     */
    MAA_FRAMEWORK_API MaaInstanceHandle
        MaaCreate(MaaInstanceCallback callback, MaaCallbackTransparentArg callback_arg);

    /**
     * @brief Free the instance.
     *
     * @param inst
     */
    MAA_FRAMEWORK_API void MaaDestroy(MaaInstanceHandle inst);

    /**
     * @brief Set options for a given instance.
     *
     * This function requires a given set of option keys and value types, otherwise this will fail.
     * See #MaaInstOptionEnum for details.
     *
     * @param inst The handle of the instance to set options for.
     * @param key The option key.
     * @param[in] value The option value.
     * @param val_size The size of the option value.
     * @return MaaBool Whether the option is set successfully.
     */
    MAA_FRAMEWORK_API MaaBool MaaSetOption(
        MaaInstanceHandle inst,
        MaaInstOption key,
        MaaOptionValue value /**< Maybe a byte array */,
        MaaOptionValueSize val_size);

    /**
     * @brief Bind the instance to an initialized resource.
     *
     * See functions in MaaResource.h about how to create a resource.
     *
     * @param inst
     * @param res
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaBindResource(MaaInstanceHandle inst, MaaResourceHandle res);

    /**
     * @brief Bind the instance to an initialized controller.
     *
     * See functions in MaaController.h about how to create a controller.
     *
     * @param inst
     * @param ctrl
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaBindController(MaaInstanceHandle inst, MaaControllerHandle ctrl);

    /**
     * @brief Check if the instance is initialized.
     *
     * @param inst
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaInited(MaaInstanceHandle inst);

    /**
     * @brief Post a task to the instance.
     *
     * The entry should be a task specified in the instance's resources.
     * The param takes the same form as the pipeline json and will override the set parameters in
     * the json file.
     *
     * @param inst
     * @param entry The entry of the task.
     * @param param The parameter of the task.
     * @return MaaTaskId The id of the task.
     */
    MAA_FRAMEWORK_API MaaTaskId
        MaaPostTask(MaaInstanceHandle inst, MaaStringView entry, MaaStringView param);

    /**
     * @brief Post a recognition to the instance.
     *
     * The entry should be a task specified in the instance's resources.
     * The param takes the same form as the pipeline json and will override the set parameters in
     * the json file.
     *
     * @param inst
     * @param entry The entry of the recognition.
     * @param param The parameter of the recognition.
     * @return MaaTaskId The id of the recognition.
     */
    MAA_FRAMEWORK_API MaaTaskId
        MaaPostRecognition(MaaInstanceHandle inst, MaaStringView entry, MaaStringView param);

    /**
     * @brief Post a action to the instance.
     *
     * The entry should be a task specified in the instance's resources.
     * The param takes the same form as the pipeline json and will override the set parameters in
     * the json file.
     *
     * @param inst
     * @param entry The entry of the action.
     * @param param The parameter of the action.
     * @return MaaTaskId The id of the action.
     */
    MAA_FRAMEWORK_API MaaTaskId
        MaaPostAction(MaaInstanceHandle inst, MaaStringView entry, MaaStringView param);

    /**
     * @brief Set the parameter of a task.
     *
     * See MaaPostTask() for details about the parameter.
     *
     * @param inst
     * @param id The id of the task.
     * @param param The parameter of the task.
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool
        MaaSetTaskParam(MaaInstanceHandle inst, MaaTaskId id, MaaStringView param);

    /**
     * @brief Get the status of a task identified by the id.
     *
     * @param inst
     * @param id
     * @return MaaStatus
     */
    MAA_FRAMEWORK_API MaaStatus MaaTaskStatus(MaaInstanceHandle inst, MaaTaskId id);

    /**
     * @brief Is maa running?
     *
     * @param inst
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaRunning(MaaInstanceHandle inst);

    /**
     * @brief Post a stop signal to the instance.
     *
     * This immediately stops the instance and all its tasks.
     *
     * @param inst
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaPostStop(MaaInstanceHandle inst);

    /**
     * @brief Get the resource handle of the instance.
     *
     * @param inst
     * @return MaaResourceHandle
     */
    MAA_FRAMEWORK_API MaaResourceHandle MaaGetResource(MaaInstanceHandle inst);

    /**
     * @brief Get the controller handle of the instance.
     *
     * @param inst
     * @return MaaControllerHandle
     */
    MAA_FRAMEWORK_API MaaControllerHandle MaaGetController(MaaInstanceHandle inst);

#ifdef __cplusplus
}
#endif
