#pragma once

#define MaaMsg_Invalid ("Invalid")

/*
    {
        id: number,
        path: string
    }
*/
#define MaaMsg_Resource_StartLoading ("Resource.StartLoading")
#define MaaMsg_Resource_LoadingCompleted ("Resource.LoadingCompleted")
#define MaaMsg_Resource_LoadingFailed ("Resource.LoadingFailed")

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
        entry: string,
        name: string,
        uuid: string,
        hash: string
    }
*/
#define MaaMsg_Task_Started ("Task.Started")
#define MaaMsg_Task_Completed ("Task.Completed")
#define MaaMsg_Task_Failed ("Task.Failed")
#define MaaMsg_Task_Stopped ("Task.Stopped")

/*
    {
        id: number,
        entry: string,
        name: string,
        uuid: string,
        hash: string,
        recognition: object,
        run_times: number,
        last_time: string,
        status: string
    }
*/
#define MaaMsg_Task_Focus_Hit ("Task.Focus.Hit")
#define MaaMsg_Task_Focus_Runout ("Task.Focus.Runout")
#define MaaMsg_Task_Focus_Completed ("Task.Focus.Completed")
