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
 * @brief The ones that have no documented payload have empty payload, which is just {}.
 *
 * @{
 */

#define MaaMsg_Invalid ("Invalid")

///
/**
 * @{
 * @brief The message for the resource loading.
 *
 * payload: {
 *      id: number,
 *      path: string
 * }
 */
#define MaaMsg_Resource_StartLoading ("Resource.StartLoading")
#define MaaMsg_Resource_LoadingCompleted ("Resource.LoadingCompleted")
#define MaaMsg_Resource_LoadingFailed ("Resource.LoadingFailed")
/// @}

/**
 * @brief The message for the UUID got.
 *
 * payload: {
 *      uuid: string
 * }
 */
#define MaaMsg_Controller_UUIDGot ("Controller.UUIDGot")

#define MaaMsg_Controller_UUIDGetFailed ("Controller.UUIDGetFailed")

/**
 * payload: {
 * }
 *
 */
#define MaaMsg_Controller_ScreencapInited ("Controller.ScreencapInited")
#define MaaMsg_Controller_ScreencapInitFailed ("Controller.ScreencapInitFailed")
#define MaaMsg_Controller_TouchInputInited ("Controller.TouchinputInited")
#define MaaMsg_Controller_TouchInputInitFailed ("Controller.TouchinputInitFailed")
#define MaaMsg_Controller_KeyInputInited ("Controller.KeyinputInited")
#define MaaMsg_Controller_KeyInputInitFailed ("Controller.KeyinputInitFailed")

/**
 * @brief Message for the controller connected.
 *
 * payload: {
 *     uuid: string
 * }
 */
#define MaaMsg_Controller_ConnectSuccess ("Controller.ConnectSuccess")

/**
 * @brief Message for the controller connect failed.
 *
 * payload: {
 *     why: string
 * }
 *
 */
#define MaaMsg_Controller_ConnectFailed ("Controller.ConnectFailed")

/**
 * @{
 * @brief Message for the controller actions.
 *
 * payload: {
 *     id: number
 * }
 */
#define MaaMsg_Controller_Action_Started ("Controller.Action.Started")
#define MaaMsg_Controller_completed ("Controller.Action.Completed")
#define MaaMsg_Controller_Action_Failed ("Controller.Action.Failed")
/// @}

/**
 * @{
 * @brief Message for the tasks.
 *
 * payload: {
 *     id: number,
 *     entry: string,
 *     name: string,
 *     uuid: string,
 *     hash: string
 * }
 */
#define MaaMsg_Tasker_Task_Started ("Tasker.Task.Started")
#define MaaMsg_Tasker_Task_Completed ("Tasker.Task.Completed")
#define MaaMsg_Tasker_Task_Failed ("Tasker.Task.Failed")
/// @}

/**
 * @{
 * @brief Message for the recognition.
 *
 * payload: {
 *     task_id: number,
 *     entry: string,
 *     uuid: string,
 *     hash: string,
 *     current: string,
 *     recognition: {
 *         reco_id: number,
 *         name: string,
 *         box: [number, number, number, number], // If not recognized, it is null
 *         detail: any,
 *     }
 * }
 */

#define MaaMsg_Task_Debug_RecognitionResult ("Task.Debug.RecognitionResult")
#define MaaMsg_Task_Debug_Hit ("Task.Debug.Hit")
/// @}


/**
 * @{
 * @brief Message for the recognition.
 *
 * payload: {
 *     task_id: number,
 *     entry: string,
 *     uuid: string,
 *     hash: string,
 *     current: string,
 *     node: {
 *         node_id: number,
 *         name: string,
 *         reco_id: number,
 *         times: number,
 *         completed: boolean,
 *     }
 * }
 */

#define MaaMsg_Task_Focus_ReadyToRun ("Task.Focus.ReadyToRun")
#define MaaMsg_Task_Focus_Completed ("Task.Focus.Completed")

#define MaaMsg_Task_Debug_ReadyToRun ("Task.Debug.ReadyToRun")
#define MaaMsg_Task_Debug_Completed ("Task.Debug.Completed")
/// @}


/**
 * @{
 * @brief Message for the task focus.
 *
 * payload: {
 *     task_id: number,
 *     entry: string,
 *     uuid: string,
 *     hash: string,
 *     current: string,
 *     list: [string],
 * }
 */
#define MaaMsg_Task_Debug_ListToRecognize ("Task.Debug.ListToRecognize")
#define MaaMsg_Task_Debug_MissAll ("Task.Debug.MissAll")
/// @}

/** @} */
