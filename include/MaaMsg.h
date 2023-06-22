#pragma once

#define MAAMSG_INVALID ("Invalid")

/*
    {
        id: number,
        path: [string, resource path]
    }
*/
#define MAAMSG_RESOURCE_START_LOADING ("Resource.StartLoading")
#define MAAMSG_RESOURCE_LOADING_COMPLETED ("Resource.LoadingCompleted")
#define MAAMSG_RESOURCE_LOADING_ERROR ("Resource.LoadingError")

/* {} */
#define MAAMSG_CONTROLLER_CONNECTED ("Controller.Connected")
#define MAAMSG_CONTROLLER_CONNECT_FAILED ("Controller.ConnectFailed")

/*
    {
        uuid: string
    }
*/
#define MAAMSG_CONTROLLER_UUID_GOT ("Controller.UUIDGot")
/* {} */
#define MAAMSG_CONTROLLER_UUID_GET_FAILED ("Controller.UUIDGetFailed")

/*
    {
        resolution: {
            width: number,
            height: number
        }
    }
*/
#define MAAMSG_CONTROLLER_RESOLUTION_GOT ("Controller.ResolutionGot")
/* {} */
#define MAAMSG_CONTROLLER_RESOLUTION_GET_FAILED ("Controller.ResolutionGetFailed")

/* {} */
#define MAAMSG_CONTROLLER_SCREENCAP_INITED ("Controller.ScreencapInited")
#define MAAMSG_CONTROLLER_SCREENCAP_INIT_FAILED ("Controller.ScreencapInitFailed")
#define MAAMSG_CONTROLLER_TOUCHINPUT_INITED ("Controller.TouchinputInited")
#define MAAMSG_CONTROLLER_TOUCHINPUT_INIT_FAILED ("Controller.TouchinputInitFailed")

/* {
        uuid: string,
        resolution: {
            width: number,
            height: number
        }
} */
#define MAAMSG_CONTROLLER_CONNECT_SUCCESS ("Controller.ConnectSuccess")

/*
    {
        id: number,
        type: [string, task type],
        uuid: [string, controller id],
        hash: [string, resource hash]
    }
*/
#define MAAMSG_TASK_STARTED ("Task.Started")
#define MAAMSG_TASK_COMPLETED ("Task.Completed")
#define MAAMSG_TASK_FAILED ("Task.Failed")
#define MAAMSG_TASK_STOPPED ("Task.Stopped")

typedef const char* MaaMsg;
