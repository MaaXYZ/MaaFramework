/**
 * @file MaaScheduler.h
 * @author
 * @brief The scheduler API.
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
     * @brief Create a scheduler.
     *
     * @param callback The callback function. See MaaNotificationCallback
     * @param callback_arg The callback arg that will be passed to the callback function.
     * @return MaaScheduler*
     */
    MAA_FRAMEWORK_API MaaScheduler* MaaSchedulerCreate(MaaNotificationCallback callback, MaaTransparentArg callback_arg);

    /**
     * @brief Free the scheduler.
     *
     * @param scheduler
     */
    MAA_FRAMEWORK_API void MaaSchedulerDestroy(MaaScheduler* scheduler);

    /**
     * @brief Set options for a given scheduler.
     *
     * This function requires a given set of option keys and value types, otherwise this will fail.
     * See #MaaSchedOptionEnum for details.
     *
     * @param scheduler The handle of the scheduler to set options for.
     * @param key The option key.
     * @param[in] value The option value.
     * @param val_size The size of the option value.
     * @return MaaBool Whether the option is set successfully.
     */
    MAA_FRAMEWORK_API MaaBool
        MaaSchedulerSetOption(MaaScheduler* scheduler, MaaSchedOption key, MaaOptionValue value, MaaOptionValueSize val_size);

    /**
     * @brief Bind the scheduler to an initialized resource.
     *
     * See functions in MaaResource.h about how to create a resource.
     *
     * @param scheduler
     * @param res
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaSchedulerBindResource(MaaScheduler* scheduler, MaaResource* res);

    /**
     * @brief Bind the scheduler to an initialized controller.
     *
     * See functions in MaaController.h about how to create a controller.
     *
     * @param scheduler
     * @param ctrl
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaSchedulerBindController(MaaScheduler* scheduler, MaaController* ctrl);

    /**
     * @brief Check if the scheduler is initialized.
     *
     * @param scheduler
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaSchedulerInited(MaaScheduler* scheduler);

    /**
     * @brief Post a task to the scheduler.
     *
     * The entry should be a task specified in the scheduler's resources.
     * The param takes the same form as the pipeline json and will override the set parameters in
     * the json file.
     *
     * @param scheduler
     * @param entry The entry of the task.
     * @param param The parameter of the task.
     * @return MaaTaskId The id of the task.
     */
    MAA_FRAMEWORK_API MaaTaskId MaaSchedulerPostTask(MaaScheduler* scheduler, const char* entry, const char* param);

    /**
     * @brief Post a recognition to the scheduler.
     *
     * The entry should be a task specified in the scheduler's resources.
     * The param takes the same form as the pipeline json and will override the set parameters in
     * the json file.
     *
     * @param scheduler
     * @param entry The entry of the recognition.
     * @param param The parameter of the recognition.
     * @return MaaTaskId The id of the recognition.
     */
    MAA_FRAMEWORK_API MaaTaskId MaaSchedulerPostRecognition(MaaScheduler* scheduler, const char* entry, const char* param);

    /**
     * @brief Post a action to the scheduler.
     *
     * The entry should be a task specified in the scheduler's resources.
     * The param takes the same form as the pipeline json and will override the set parameters in
     * the json file.
     *
     * @param scheduler
     * @param entry The entry of the action.
     * @param param The parameter of the action.
     * @return MaaTaskId The id of the action.
     */
    MAA_FRAMEWORK_API MaaTaskId MaaSchedulerPostAction(MaaScheduler* scheduler, const char* entry, const char* param);

    /**
     * @brief Set the parameter of a task.
     *
     * See MaaPostTask() for details about the parameter.
     *
     * @param scheduler
     * @param id The id of the task.
     * @param param The parameter of the task.
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaSchedulerSetParam(MaaScheduler* scheduler, MaaTaskId id, const char* param);

    /**
     * @brief Get the status of a task identified by the id.
     *
     * @param scheduler
     * @param id
     * @return MaaStatus
     */
    MAA_FRAMEWORK_API MaaStatus MaaSchedulerStatus(MaaScheduler* scheduler, MaaTaskId id);

    /**
     * @brief Wait for a task to complete.
     *
     * @param scheduler
     * @param id
     * @return MaaStatus
     */
    MAA_FRAMEWORK_API MaaStatus MaaSchedulerWait(MaaScheduler* scheduler, MaaTaskId id);

    /**
     * @brief Is maa running?
     *
     * @param scheduler
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaSchedulerRunning(MaaScheduler* scheduler);

    /**
     * @brief Post a stop signal to the scheduler.
     *
     * This immediately stops the scheduler and all its tasks.
     *
     * @param scheduler
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaSchedulerPostStop(MaaScheduler* scheduler);

    /**
     * @brief Get the resource handle of the scheduler.
     *
     * @param scheduler
     * @return MaaResource*
     */
    MAA_FRAMEWORK_API MaaResource* MaaSchedulerGetResource(MaaScheduler* scheduler);

    /**
     * @brief Get the controller handle of the scheduler.
     *
     * @param scheduler
     * @return MaaController*
     */
    MAA_FRAMEWORK_API MaaController* MaaSchedulerGetController(MaaScheduler* scheduler);

#ifdef __cplusplus
}
#endif
