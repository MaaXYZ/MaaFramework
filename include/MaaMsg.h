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
