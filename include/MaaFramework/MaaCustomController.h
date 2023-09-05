#pragma once

#include "MaaDef.h"
#include "MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct MaaCustomControllerAPI
    {
        MaaBool (*set_option)(MaaCtrlOption key, MaaStringView value);

        MaaBool (*connect)(void);
        MaaBool (*click)(int32_t x, int32_t y);
        MaaBool (*swipe)(int32_t* x_steps_buff, int32_t* y_steps_buff, int32_t* step_delay_buff, MaaSize buff_size);
        MaaBool (*press_key)(int32_t keycode);

        MaaBool (*start_app)(MaaStringView package_name);
        MaaBool (*stop_app)(MaaStringView package_name);

        MaaBool (*get_resolution)(int32_t* width, int32_t* height);
        // if buff is null, screencap and return image size; else return written size
        MaaSize (*get_image)(uint8_t* buff, MaaSize buff_size);
        // if buff is null, return uuid string size; else return written size
        MaaSize (*get_uuid)(char* buff, MaaSize buff_size);
    };

#ifdef __cplusplus
}
#endif
