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

    /**
     * @brief Create a tasker.
     *
     * @param callback The callback function. See MaaNotificationCallback
     * @param callback_arg The callback arg that will be passed to the callback function.
     * @return MaaTasker*
     */
    MAA_FRAMEWORK_API MaaTasker* MaaTaskerCreate(MaaNotificationCallback callback, void* callback_arg);

    /**
     * @brief Free the tasker.
     *
     * @param tasker
     */
    MAA_FRAMEWORK_API void MaaTaskerDestroy(MaaTasker* tasker);

    /**
     * @brief Set options for a given tasker.
     *
     * This function requires a given set of option keys and value types, otherwise this will fail.
     * See #MaaTaskerOptionEnum for details.
     *
     * @param tasker The handle of the tasker to set options for.
     * @param key The option key.
     * @param[in] value The option value.
     * @param val_size The size of the option value.
     * @return MaaBool Whether the option is set successfully.
     */
    MAA_FRAMEWORK_API MaaBool MaaTaskerSetOption(MaaTasker* tasker, MaaTaskerOption key, MaaOptionValue value, MaaOptionValueSize val_size);

    /**
     * @brief Bind the tasker to an initialized resource.
     *
     * See functions in MaaResource.h about how to create a resource.
     *
     * @param tasker
     * @param res
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaTaskerBindResource(MaaTasker* tasker, MaaResource* res);

    /**
     * @brief Bind the tasker to an initialized controller.
     *
     * See functions in MaaController.h about how to create a controller.
     *
     * @param tasker
     * @param ctrl
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaTaskerBindController(MaaTasker* tasker, MaaController* ctrl);

    /**
     * @brief Check if the tasker is initialized.
     *
     * @param tasker
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaTaskerInited(MaaTasker* tasker);

    /**
     * @brief Post a task to the tasker.
     *
     * The entry should be a task specified in the tasker's resources.
     * The param takes the same form as the pipeline json and will override the set parameters in
     * the json file.
     *
     * @param tasker
     * @param entry The entry of the task.
     * @param pipeline_override The override of the pipeline.
     * @return MaaTaskId The id of the task.
     */
    MAA_FRAMEWORK_API MaaTaskId MaaTaskerPostPipeline(MaaTasker* tasker, const char* entry, const char* pipeline_override);

    /**
     * @brief Post a recognition to the tasker.
     *
     * The entry should be a task specified in the tasker's resources.
     * The param takes the same form as the pipeline json and will override the set parameters in
     * the json file.
     *
     * @param tasker
     * @param entry The entry of the recognition.
     * @param pipeline_override The override of the pipeline.
     * @return MaaTaskId The id of the recognition.
     */
    MAA_FRAMEWORK_API MaaTaskId MaaTaskerPostRecognition(MaaTasker* tasker, const char* entry, const char* pipeline_override);

    /**
     * @brief Post a action to the tasker.
     *
     * The entry should be a task specified in the tasker's resources.
     * The param takes the same form as the pipeline json and will override the set parameters in
     * the json file.
     *
     * @param tasker
     * @param entry The entry of the action.
     * @param pipeline_override The override of the pipeline.
     * @return MaaTaskId The id of the action.
     */
    MAA_FRAMEWORK_API MaaTaskId MaaTaskerPostAction(MaaTasker* tasker, const char* entry, const char* pipeline_override);

    /**
     * @brief Set the parameter of a task.
     *
     * See MaaPostTask() for details about the parameter.
     *
     * @param tasker
     * @param id The id of the task.
     * @param pipeline_override The override of the pipeline.
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaTaskerOverridePipeline(MaaTasker* tasker, MaaTaskId id, const char* pipeline_override);

    /**
     * @brief Get the status of a task identified by the id.
     *
     * @param tasker
     * @param id
     * @return MaaStatus
     */
    MAA_FRAMEWORK_API MaaStatus MaaTaskerStatus(MaaTasker* tasker, MaaTaskId id);

    /**
     * @brief Wait for a task to complete.
     *
     * @param tasker
     * @param id
     * @return MaaStatus
     */
    MAA_FRAMEWORK_API MaaStatus MaaTaskerWait(MaaTasker* tasker, MaaTaskId id);

    /**
     * @brief Is maa running?
     *
     * @param tasker
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaTaskerRunning(MaaTasker* tasker);

    /**
     * @brief Post a stop signal to the tasker.
     *
     * This immediately stops the tasker and all its tasks.
     *
     * @param tasker
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaTaskerPostStop(MaaTasker* tasker);

    /**
     * @brief Get the resource handle of the tasker.
     *
     * @param tasker
     * @return MaaResource*
     */
    MAA_FRAMEWORK_API MaaResource* MaaTaskerGetResource(MaaTasker* tasker);

    /**
     * @brief Get the controller handle of the tasker.
     *
     * @param tasker
     * @return MaaController*
     */
    MAA_FRAMEWORK_API MaaController* MaaTaskerGetController(MaaTasker* tasker);

    MAA_FRAMEWORK_API MaaBool MaaTaskerClearCache(MaaTasker* tasker);

    /**
     * @param[out] hit
     */
    MAA_FRAMEWORK_API MaaBool MaaTaskerGetRecognitionDetail(
        MaaTasker* tasker,
        MaaRecoId reco_id,
        /* out */ MaaStringBuffer* name,
        /* out */ MaaBool* hit,
        /* out */ MaaRect* box,
        /* out */ MaaStringBuffer* detail_json,
        /* out */ MaaImageBuffer* raw,
        /* out */ MaaImageListBuffer* draws);

    /**
     * @param[out] reco_id
     * @param[out] times
     * @param[out] completed
     */
    MAA_FRAMEWORK_API MaaBool MaaTaskerGetNodeDetail(
        MaaTasker* tasker,
        MaaNodeId node_id,
        /* out */ MaaStringBuffer* name,
        /* out */ MaaRecoId* reco_id,
        /* out */ MaaSize* times,
        /* out */ MaaBool* completed);

    /**
     * @param[out] node_id_list
     * @param[in, out] node_id_list_size
     */
    MAA_FRAMEWORK_API MaaBool MaaTaskerGetTaskDetail(
        MaaTasker* tasker,
        MaaTaskId task_id,
        /* out */ MaaStringBuffer* entry,
        /* out */ MaaNodeId* node_id_list /**< array */,
        /* in & out */ MaaSize* node_id_list_size);

#ifdef __cplusplus
}
#endif
