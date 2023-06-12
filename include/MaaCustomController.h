#pragma once

#include "MaaDef.h"
#include "MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif
    struct MAAAPI_PORT MaaCustomControllerAPI
    {
        MaaBool (*set_option)(MaaCtrlOption key, MaaString value);

        MaaBool (*connect)(void);
        MaaBool (*click)(int32_t x, int32_t y);
        MaaBool (*swipe)(int32_t* x_steps_buff, int32_t* y_steps_buff, int32_t* step_delay_buff, MaaSize buff_size);
        MaaBool (*press_key)(int32_t keycode);

        MaaBool (*app_start)(MaaString package_name);
        // if activity is empty, kill current app
        MaaBool (*app_stop)(MaaString package_name);

        // if buff is null, screencap and return image size; else return write size
        MaaSize (*get_image)(uchar* buff, MaaSize buff_size);
        // if buff is null, return uuid string size; else return write size
        MaaSize (*get_uuid)(char* buff, MaaSize buff_size);
    };

#ifdef __cplusplus
}
#endif
