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
 * @brief Message for the task.
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
 * @brief Message for the next list of node.
 *
 * payload: {
 *     task_id: number,
 *     name: string,
 *     list: string[],
 *     focus: any,
 * }
 */
#define MaaMsg_Node_NextList_Starting ("Node.NextList.Starting")
#define MaaMsg_Node_NextList_Succeeded ("Node.NextList.Succeeded")
#define MaaMsg_Node_NextList_Failed ("Node.NextList.Failed")
/// @}

/**
 * @{
 * @brief Message for the recognition list of node.
 *
 * payload: {
 *     task_id: number,
 *     reco_id: number,
 *     name: string,
 *     focus: any,
 * }
 */
#define MaaMsg_Node_Recognition_Starting ("Node.Recognition.Starting")
#define MaaMsg_Node_Recognition_Succeeded ("Node.Recognition.Succeeded")
#define MaaMsg_Node_Recognition_Failed ("Node.Recognition.Failed")
/// @}

/**
 * @{
 * @brief Message for the action of node.
 *
 * payload: {
 *     task_id: number,
 *     node_id: number,
 *     name: string,
 *     focus: any,
 * }
 */
#define MaaMsg_Node_Action_Starting ("Node.Action.Starting")
#define MaaMsg_Node_Action_Succeeded ("Node.Action.Succeeded")
#define MaaMsg_Node_Action_Failed ("Node.Action.Failed")
/// @}

/** @} */
