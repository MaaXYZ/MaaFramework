/**
 * @file MaaMsg.h
 * @author
 * @brief Contains the callback message definitions.
 *
 * A callback consists of a message and a payload. The message is a string that indicates the type
 * of the message. The payload is a JSON object that contains the details of the message.
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

/**
 * @defgroup MaaMsg Message Definitions
 *
 * @{
 */

/**
 * @{
 * @brief The message for the resource loading.
 *
 * payload: {
 *      res_id: number,
 *      hash: string,
 *      path: string,
 * }
 */
#define MaaMsg_Resource_Loading_Starting ("Resource.Loading.Starting")
#define MaaMsg_Resource_Loading_Succeeded ("Resource.Loading.Succeeded")
#define MaaMsg_Resource_Loading_Failed ("Resource.Loading.Failed")
/// @}

/**
 * @{
 * @brief Message for the controller actions.
 *
 * payload: {
 *     ctrl_id: number,
 *     uuid: string,
 *     action: string,
 * }
 */
#define MaaMsg_Controller_Action_Starting ("Controller.Action.Starting")
#define MaaMsg_Controller_Action_Succeeded ("Controller.Action.Succeeded")
#define MaaMsg_Controller_Action_Failed ("Controller.Action.Failed")
/// @}

/**
 * @{
 * @brief Message for the tasks.
 *
 * payload: {
 *     task_id: number,
 *     entry: string,
 *     uuid: string,
 *     hash: string
 * }
 */
#define MaaMsg_Tasker_Task_Starting ("Tasker.Task.Starting")
#define MaaMsg_Tasker_Task_Succeeded ("Tasker.Task.Succeeded")
#define MaaMsg_Tasker_Task_Failed ("Tasker.Task.Failed")
/// @}

/**
 * @{
 * @brief Message for the Recognition List.
 *
 * payload: {
 *     task_id: number,
 *     name: string,
 *     list: string[],
 * }
 */
#define MaaMsg_Task_NextList_Starting ("Task.NextList.Starting")
#define MaaMsg_Task_NextList_Succeeded ("Task.NextList.Succeeded")
#define MaaMsg_Task_NextList_Failed ("Task.NextList.Failed")
/// @}

/**
 * @{
 * @brief Message for the recognition list.
 *
 * payload: {
 *     task_id: number,
 *     reco_id: number,
 *     name: string,
 * }
 */
#define MaaMsg_Task_Recognition_Starting ("Task.Recognition.Starting")
#define MaaMsg_Task_Recognition_Succeeded ("Task.Recognition.Succeeded")
#define MaaMsg_Task_Recognition_Failed ("Task.Recognition.Failed")
/// @}

/**
 * @{
 * @brief Message for the task action.
 *
 * payload: {
 *     task_id: number,
 *     node_id: number,
 *     name: string,
 * }
 */
#define MaaMsg_Task_Action_Starting ("Task.Action.Starting")
#define MaaMsg_Task_Action_Succeeded ("Task.Action.Succeeded")
#define MaaMsg_Task_Action_Failed ("Task.Action.Failed")
/// @}

/** @} */
