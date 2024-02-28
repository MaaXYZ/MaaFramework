/**
 * @file MaaInstance.h
 * @author
 * @brief The instance API.
 *
 * @copyright Copyright (c) 2024
 *
 */

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
    MaaInstanceHandle MAA_FRAMEWORK_API MaaCreate(MaaInstanceCallback callback, MaaCallbackTransparentArg callback_arg);

    /**
     * @brief Free the instance.
     *
     * @param inst
     */
    void MAA_FRAMEWORK_API MaaDestroy(MaaInstanceHandle inst);

    /**
     * @brief Set options for a given instance.
     *
     * This function requires a given set of option keys and value types, otherwise this will fail. See
     * #MaaInstOptionEnum for details.
     *
     * @param inst The handle of the instance to set options for.
     * @param key The option key.
     * @param value The option value.
     * @param val_size The size of the option value.
     * @return MaaBool Whether the option is set successfully.
     */
    MaaBool MAA_FRAMEWORK_API MaaSetOption(MaaInstanceHandle inst, MaaInstOption key, MaaOptionValue value,
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
    MaaBool MAA_FRAMEWORK_API MaaBindResource(MaaInstanceHandle inst, MaaResourceHandle res);

    /**
     * @brief Bind the instance to an initialized controller.
     *
     * See functions in MaaController.h about how to create a controller.
     *
     * @param inst
     * @param ctrl
     * @return MaaBool
     */
    MaaBool MAA_FRAMEWORK_API MaaBindController(MaaInstanceHandle inst, MaaControllerHandle ctrl);

    /**
     * @brief Check if the instance is initialized.
     *
     * @param inst
     * @return MaaBool
     */
    MaaBool MAA_FRAMEWORK_API MaaInited(MaaInstanceHandle inst);

    /**
     * @brief Register a custom recognizer to the instance.
     *
     * See MaaCustomRecognizer.h for details about how to create a custom recognizer.
     *
     * @param inst
     * @param name The name of the recognizer that will be used to reference it.
     * @param recognizer
     * @param recognizer_arg
     * @return MaaBool
     */
    MaaBool MAA_FRAMEWORK_API MaaRegisterCustomRecognizer(MaaInstanceHandle inst, MaaStringView name,
                                                          MaaCustomRecognizerHandle recognizer,
                                                          MaaTransparentArg recognizer_arg);

    /**
     * @brief Unregister a custom recognizer from the instance.
     *
     * @param inst
     * @param name The name of the recognizer when it was registered.
     * @return MaaBool
     */
    MaaBool MAA_FRAMEWORK_API MaaUnregisterCustomRecognizer(MaaInstanceHandle inst, MaaStringView name);

    /**
     * @brief Clear all custom recognizers registered to the instance.
     *
     * @param inst
     * @return MaaBool
     */
    MaaBool MAA_FRAMEWORK_API MaaClearCustomRecognizer(MaaInstanceHandle inst);

    /**
     * @brief Register a custom action to the instance.
     *
     * See MaaCustomAction.h for details about how to create a custom action.
     *
     * @param inst
     * @param name The name of the action that will be used to reference it.
     * @param action
     * @param action_arg
     * @return MaaBool
     */
    MaaBool MAA_FRAMEWORK_API MaaRegisterCustomAction(MaaInstanceHandle inst, MaaStringView name,
                                                      MaaCustomActionHandle action, MaaTransparentArg action_arg);

    /**
     * @brief Unregister a custom action from the instance.
     *
     * @param inst
     * @param name The name of the action when it was registered.
     * @return MaaBool
     */
    MaaBool MAA_FRAMEWORK_API MaaUnregisterCustomAction(MaaInstanceHandle inst, MaaStringView name);

    /**
     * @brief Clear all custom actions registered to the instance.
     *
     * @param inst
     * @return MaaBool
     */
    MaaBool MAA_FRAMEWORK_API MaaClearCustomAction(MaaInstanceHandle inst);

    /**
     * @brief Post a task to the instance.
     *
     * The entry should be a task specified in the instance's resources.
     * The param takes the same form as the pipeline json and will override the set parameters in the json file.
     *
     * @param inst
     * @param entry The entry of the task.
     * @param param The parameter of the task.
     * @return MaaTaskId The id of the task.
     */
    MaaTaskId MAA_FRAMEWORK_API MaaPostTask(MaaInstanceHandle inst, MaaStringView entry, MaaStringView param);

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
    MaaBool MAA_FRAMEWORK_API MaaSetTaskParam(MaaInstanceHandle inst, MaaTaskId id, MaaStringView param);

    /**
     * @brief Get the status of a task identified by the id.
     *
     * @param inst
     * @param id
     * @return MaaStatus
     */
    MaaStatus MAA_FRAMEWORK_API MaaTaskStatus(MaaInstanceHandle inst, MaaTaskId id);

    /**
     * @brief Wait for a task to finish.
     *
     * @param inst
     * @param id
     * @return MaaStatus
     */
    MaaStatus MAA_FRAMEWORK_API MaaWaitTask(MaaInstanceHandle inst, MaaTaskId id);

    /**
     * @brief Wait for all tasks to finish.
     *
     * @param inst
     * @return MaaBool
     */
    MaaBool MAA_FRAMEWORK_API MaaTaskAllFinished(MaaInstanceHandle inst);

    /**
     * @brief Post a stop signal to the instance.
     *
     * This immediately stops the instance and all its tasks.
     *
     * @param inst
     * @return MaaBool
     */
    MaaBool MAA_FRAMEWORK_API MaaPostStop(MaaInstanceHandle inst);

    /// \deprecated Use MaaPostStop() instead.
    MaaBool MAA_FRAMEWORK_API MaaStop(MaaInstanceHandle inst);

    /**
     * @brief Get the resource handle of the instance.
     *
     * @param inst
     * @return MaaResourceHandle
     */
    MaaResourceHandle MAA_FRAMEWORK_API MaaGetResource(MaaInstanceHandle inst);

    /**
     * @brief Get the controller handle of the instance.
     *
     * @param inst
     * @return MaaControllerHandle
     */
    MaaControllerHandle MAA_FRAMEWORK_API MaaGetController(MaaInstanceHandle inst);

#ifdef __cplusplus
}
#endif
