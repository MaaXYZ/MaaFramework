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

///
/**
 * @brief The message for the resource loading.
 *
 * payload: {
 *      res_id: number,
 *      hash: string,
 *      path: string,
 * }
 */
#define MaaMsg_Resource_Loading_Started ("Resource.Loading.Started")
#define MaaMsg_Resource_Loading_Completed ("Resource.Loading.Completed")
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
#define MaaMsg_Controller_Action_Started ("Controller.Action.Started")
#define MaaMsg_Controller_Action_Completed ("Controller.Action.Completed")
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
#define MaaMsg_Tasker_Task_Started ("Tasker.Task.Started")
#define MaaMsg_Tasker_Task_Completed ("Tasker.Task.Completed")
#define MaaMsg_Tasker_Task_Failed ("Tasker.Task.Failed")
/// @}

/** @} */
