#include "MaaFramework/Instance/MaaContext.h"

#include "API/MaaTypes.h"
#include "Buffer/ImageBuffer.hpp"
#include "Buffer/StringBuffer.hpp"
#include "Utils/Logger.h"

MaaTaskId MaaContextRunPipeline(MaaContext* context, const char* task_name, const char* pipeline_override)
{
    LogFunc << VAR_VOIDP(context) << VAR(task_name) << VAR(param);

    if (!context) {
        LogError << "handle is null";
        return false;
    }

    return context->run_pipeline(task_name, param);
}

MaaTaskId MaaContextRunRecognition(MaaContext* context, const char* task_name, const char* task_param, MaaImageBuffer* image)
{
    LogFunc << VAR_VOIDP(context) << VAR_VOIDP(image) << VAR(task_name) << VAR(task_param);

    if (!context || !image) {
        LogError << "handle is null";
        return false;
    }

    return context->run_recognition(task_name, task_param, image->get());
}

MaaTaskId
    MaaContextRunAction(MaaContext* context, const char* task_name, const char* task_param, MaaRect* cur_box, const char* cur_rec_detail)
{
    LogFunc << VAR_VOIDP(context) << VAR(task_name) << VAR(task_param) << VAR(cur_box) << VAR(cur_rec_detail);

    if (!context) {
        LogError << "handle is null";
        return false;
    }

    cv::Rect cvbox {};
    if (cur_box) {
        cvbox.x = cur_box->x;
        cvbox.y = cur_box->y;
        cvbox.width = cur_box->width;
        cvbox.height = cur_box->height;
    }
    return context->run_action(task_name, task_param, cvbox, cur_rec_detail);
}

MaaTaskId MaaContextGetTaskId(MaaContext* context)
{
    if (!context) {
        LogError << "handle is null";
        return false;
    }

    return context->task_id();
}

MaaTasker* MaaContextGetTasker(MaaContext* context)
{
    if (!context) {
        LogError << "handle is null";
        return false;
    }

    return context->tasker();
}
