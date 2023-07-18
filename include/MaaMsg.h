#pragma once

#define MaaMsg_Invalid ("Invalid")

/*
    {
        id: number,
        path: [string, resource path]
    }
*/
#define MaaMsg_Resource_StartLoading ("Resource.StartLoading")
#define MaaMsg_Resource_LoadingCompleted ("Resource.LoadingCompleted")
#define MaaMsg_Resource_LoadingError ("Resource.LoadingError")

/*
    {
        uuid: string
    }
*/
#define MaaMsg_Controller_UUIDGot ("Controller.UUIDGot")
/* {} */
#define MaaMsg_Controller_UUIDGetFailed ("Controller.UUIDGetFailed")

/*
    {
        resolution: {
            width: number,
            height: number
        }
    }
*/
#define MaaMsg_Controller_ResolutionGot ("Controller.ResolutionGot")
/* {} */
#define MaaMsg_Controller_ResolutionGetFailed ("Controller.ResolutionGetFailed")

/* {} */
#define MaaMsg_Controller_ScreencapInited ("Controller.ScreencapInited")
#define MaaMsg_Controller_ScreencapInitFailed ("Controller.ScreencapInitFailed")
#define MaaMsg_Controller_TouchInputInited ("Controller.TouchinputInited")
#define MaaMsg_Controller_TouchInputInitFailed ("Controller.TouchinputInitFailed")

/* {
        uuid: string,
        resolution: {
            width: number,
            height: number
        }
} */
#define MaaMsg_Controller_ConnectSuccess ("Controller.ConnectSuccess")
/* {
*      "why": string
} */
#define MaaMsg_Controller_ConnectFailed ("Controller.ConnectFailed")

/*
 * {
 *    id: number
 * }
 */
#define MaaMsg_Controller_Action_Started ("Controller.Action.Started")
#define MaaMsg_Controller_Action_Completed ("Controller.Action.Completed")
#define MaaMsg_Controller_Action_Failed ("Controller.Action.Failed")
// #define MaaMsg_Controller_Action_Stopped ("Controller.Action.Stopped")

/*
    {
        id: number,
        uuid: [string, controller id],
        hash: [string, resource hash]
    }
*/
#define MaaMsg_Task_Started ("Task.Started")
#define MaaMsg_Task_Completed ("Task.Completed")
#define MaaMsg_Task_Failed ("Task.Failed")
#define MaaMsg_Task_Stopped ("Task.Stopped")
