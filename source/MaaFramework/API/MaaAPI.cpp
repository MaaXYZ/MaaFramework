#include "MaaFramework/MaaAPI.h"

#include <meojson/json.hpp>

#include "Buffer/ImageBuffer.hpp"
#include "Buffer/StringBuffer.hpp"
#include "ControlUnit/ControlUnitAPI.h"
#include "Controller/AdbController.h"
#include "Controller/CustomController.h"
#include "Controller/CustomThriftController.h"
#include "Instance/InstanceMgr.h"
#include "Option/GlobalOptionMgr.h"
#include "Resource/ResourceMgr.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

#pragma message("MAA_VERSION: " MAA_VERSION)

MaaStringView MaaVersion()
{
    return MAA_VERSION;
}

MaaStringBufferHandle MaaCreateStringBuffer()
{
    return new MAA_NS::StringBuffer;
}

void MaaDestroyStringBuffer(MaaStringBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return;
    }

    delete handle;
}

MaaStringView MaaGetString(MaaStringBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return handle->data();
}

MaaSize MaaGetStringSize(MaaStringBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->size();
}

MaaBool MaaSetString(MaaStringBufferHandle handle, MaaStringView str)
{
    if (!handle || !str) {
        LogError << "handle is null";
        return false;
    }

    handle->set(str);
    return true;
}

MaaBool MaaSetStringEx(MaaStringBufferHandle handle, MaaStringView str, MaaSize size)
{
    if (!handle || !str) {
        LogError << "handle is null";
        return false;
    }

    handle->set(std::string(str, size));
    return true;
}

MaaImageBufferHandle MaaCreateImageBuffer()
{
    return new MAA_NS::ImageBuffer;
}

void MaaDestroyImageBuffer(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return;
    }

    delete handle;
}

void* MaaGetImageRawData(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return handle->raw_data();
}

int32_t MaaGetImageWidth(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->width();
}

int32_t MaaGetImageHeight(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->height();
}

int32_t MaaGetImageType(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->type();
}

MaaBool MaaSetImageRawData(MaaImageBufferHandle handle, MaaImageRawData data, int32_t width, int32_t height,
                           int32_t type)
{
    if (!handle || !data) {
        LogError << "handle is null";
        return false;
    }

    cv::Mat img(height, width, type, data);
    handle->set(img);
    return true;
}

uint8_t* MaaGetImageEncoded(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return handle->encoded();
}

MaaSize MaaGetImageEncodedSize(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->encoded_size();
}

MaaBool MaaSetImageEncoded(MaaImageBufferHandle handle, MaaImageEncodedData data, MaaSize size)
{
    if (!handle || !data) {
        LogError << "handle is null";
        return false;
    }

    cv::Mat img = cv::imdecode({ data, static_cast<int>(size) }, cv::IMREAD_COLOR);
    handle->set(img);
    return true;
}

MaaBool MaaSetGlobalOption(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    return MAA_NS::GlobalOptionMgr::get_instance().set_option(key, value, val_size);
}

MaaResourceHandle MaaResourceCreate(MaaResourceCallback callback, MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    return new MAA_RES_NS::ResourceMgr(callback, callback_arg);
}

void MaaResourceDestroy(MaaResourceHandle res)
{
    LogFunc << VAR_VOIDP(res);

    if (res == nullptr) {
        LogError << "handle is null";
        return;
    }

    delete res;
}

MaaResId MaaResourcePostResource(MaaResourceHandle res, MaaStringView path)
{
    LogFunc << VAR_VOIDP(res) << VAR(path);

    if (!res) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return res->post_resource(MAA_NS::path(path));
}

MaaStatus MaaResourceStatus(MaaResourceHandle res, MaaResId id)
{
    // LogFunc << VAR_VOIDP(res) << VAR(id);

    if (!res) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return res->status(id);
}

MaaStatus MaaResourceWait(MaaResourceHandle res, MaaResId id)
{
    // LogFunc << VAR_VOIDP(res) << VAR(id);

    if (!res) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return res->wait(id);
}

MaaBool MaaResourceLoaded(MaaResourceHandle res)
{
    // LogFunc << VAR_VOIDP(res);

    if (!res) {
        LogError << "handle is null";
        return false;
    }

    return res->loaded();
}

MaaBool MaaResourceSetOption(MaaResourceHandle res, MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(res) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!res) {
        LogError << "handle is null";
        return false;
    }

    return res->set_option(key, value, val_size);
}

MaaBool MaaResourceGetHash(MaaResourceHandle res, MaaStringBufferHandle buff)
{
    if (!res || !buff) {
        LogError << "handle is null";
        return false;
    }

    buff->set(res->get_hash());
    return true;
}

MaaControllerHandle MaaAdbControllerCreate(MaaStringView adb_path, MaaStringView address, MaaAdbControllerType type,
                                           MaaStringView config, MaaControllerCallback callback,
                                           MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR(adb_path) << VAR(address) << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    auto unit_mgr = MAA_CTRL_UNIT_NS::create_adb_controller_unit(adb_path, address, type, config);
    if (!unit_mgr) {
        LogError << "Failed to create controller unit";
        return nullptr;
    }

    return new MAA_CTRL_NS::AdbController(adb_path, address, std::move(unit_mgr), callback, callback_arg);
}

MaaControllerHandle MaaCustomControllerCreate(MaaCustomControllerHandle handle, MaaControllerCallback callback,
                                              MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR(handle) << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return new MAA_CTRL_NS::CustomController(handle, callback, callback_arg);
}

MaaControllerHandle MaaThriftControllerCreate(MaaStringView param, MaaControllerCallback callback,
                                              MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR(param) << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

#ifdef WITH_THRIFT

    try {
        return new MAA_CTRL_NS::CustomThriftController(param, callback, callback_arg);
    }
    catch (const std::exception& e) {
        LogError << "Failed to create thrift controller: " << e.what();
        return nullptr;
    }

#else

#pragma message("The build without thrift")

    LogError << "The build without thrift";
    return nullptr;

#endif // WITH_THRIFT
}

void MaaControllerDestroy(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (ctrl == nullptr) {
        LogError << "handle is null";
        return;
    }

    ctrl->on_stop();
    delete ctrl;
}

MaaBool MaaControllerSetOption(MaaControllerHandle ctrl, MaaCtrlOption key, MaaOptionValue value,
                               MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!ctrl) {
        LogError << "handle is null";
        return false;
    }

    return ctrl->set_option(key, value, val_size);
}

MaaCtrlId MaaControllerPostConnection(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (!ctrl) {
        LogError << "handle is null";
        return false;
    }

    return ctrl->post_connection();
}

MaaCtrlId MaaControllerPostClick(MaaControllerHandle ctrl, int32_t x, int32_t y)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(x) << VAR(y);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_click(x, y);
}

MaaCtrlId MaaControllerPostSwipe(MaaControllerHandle ctrl, int32_t* x_steps_buff, int32_t* y_steps_buff,
                                 int32_t* step_delay_buff, MaaSize buff_size)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR_VOIDP(x_steps_buff) << VAR_VOIDP(y_steps_buff) << VAR_VOIDP(step_delay_buff)
            << VAR(buff_size);

    if (!ctrl || !x_steps_buff || !y_steps_buff || !step_delay_buff || buff_size < 2) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    std::vector<int32_t> x_steps(x_steps_buff, x_steps_buff + buff_size);
    std::vector<int32_t> y_steps(y_steps_buff, y_steps_buff + buff_size);
    std::vector<int32_t> step_delay(step_delay_buff, step_delay_buff + buff_size);

    return ctrl->post_swipe(std::move(x_steps), std::move(y_steps), std::move(step_delay));
}

MaaCtrlId MaaControllerPostScreencap(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_screencap();
}

MaaStatus MaaControllerStatus(MaaControllerHandle ctrl, MaaCtrlId id)
{
    // LogFunc << VAR_VOIDP(ctrl) << VAR(id);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return ctrl->status(id);
}

MaaStatus MaaControllerWait(MaaControllerHandle ctrl, MaaCtrlId id)
{
    // LogFunc << VAR_VOIDP(ctrl) << VAR(id);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return ctrl->wait(id);
}

MaaBool MaaControllerConnected(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (!ctrl) {
        LogError << "handle is null";
        return false;
    }

    return ctrl->connected();
}

MaaBool MaaControllerGetImage(MaaControllerHandle ctrl, MaaImageBufferHandle buffer)
{
    if (!ctrl || !buffer) {
        LogError << "handle is null";
        return false;
    }

    buffer->set(ctrl->get_image());
    return true;
}

MaaBool MaaControllerGetUUID(MaaControllerHandle ctrl, MaaStringBufferHandle buffer)
{
    if (!ctrl || !buffer) {
        LogError << "handle is null";
        return false;
    }

    buffer->set(ctrl->get_uuid());
    return true;
}

MaaInstanceHandle MaaCreate(MaaInstanceCallback callback, MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    return new MAA_NS::InstanceMgr(callback, callback_arg);
}

void MaaDestroy(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (inst == nullptr) {
        LogError << "handle is null";
        return;
    }

    delete inst;
}

MaaBool MaaSetOption(MaaInstanceHandle inst, MaaInstOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(inst) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->set_option(key, value, val_size);
}

MaaBool MaaBindResource(MaaInstanceHandle inst, MaaResourceHandle res)
{
    LogFunc << VAR_VOIDP(inst) << VAR_VOIDP(res);

    if (!inst || !res) {
        LogError << "handle is null";
        return false;
    }

    return inst->bind_resource(res);
}

MaaBool MaaBindController(MaaInstanceHandle inst, MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(inst) << VAR_VOIDP(ctrl);

    if (!inst || !ctrl) {
        LogError << "handle is null";
        return false;
    }

    return inst->bind_controller(ctrl);
}

MaaBool MaaInited(MaaInstanceHandle inst)
{
    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->inited();
}

MaaBool MaaRegisterCustomRecognizer(MaaInstanceHandle inst, MaaStringView name, MaaCustomRecognizerHandle recognizer)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name) << VAR_VOIDP(recognizer);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->register_custom_recognizer(name, recognizer);
}

MaaBool MaaUnregisterCustomRecognizer(MaaInstanceHandle inst, MaaStringView name)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->unregister_custom_recognizer(name);
}

MaaBool MaaClearCustomRecognizer(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    inst->clear_custom_recognizer();
    return true;
}

MaaBool MaaRegisterCustomAction(MaaInstanceHandle inst, MaaStringView name, MaaCustomActionHandle action)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name) << VAR_VOIDP(action);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->register_custom_action(name, action);
}

MaaBool MaaUnregisterCustomAction(MaaInstanceHandle inst, MaaStringView name)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->unregister_custom_action(name);
}

MaaBool MaaClearCustomAction(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    inst->clear_custom_action();

    return true;
}

MaaTaskId MaaPostTask(MaaInstanceHandle inst, MaaStringView entry, MaaStringView param)
{
    LogFunc << VAR_VOIDP(inst) << VAR(entry) << VAR(param);

    if (!inst) {
        LogError << "handle is null";
        return MaaInvalidId;
    }
    return inst->post_task(entry, param);
}

MaaBool MaaSetTaskParam(MaaInstanceHandle inst, MaaTaskId id, MaaStringView param)
{
    LogFunc << VAR_VOIDP(inst) << VAR(id) << VAR(param);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }
    return inst->set_task_param(id, param);
}

MaaStatus MaaTaskStatus(MaaInstanceHandle inst, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(inst) << VAR(id);

    if (!inst) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }
    return inst->status(id);
}

MaaStatus MaaWaitTask(MaaInstanceHandle inst, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(inst) << VAR(id);

    if (!inst) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }
    return inst->wait(id);
}

MaaBool MaaTaskAllFinished(MaaInstanceHandle inst)
{
    // LogFunc << VAR_VOIDP(inst) << VAR(id);
    if (!inst) {
        LogError << "handle is null";
        return false;
    }
    return inst->all_finished();
}

void MaaStop(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return;
    }

    inst->stop();
}

MaaResourceHandle MaaGetResource(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return nullptr;
    }

    return inst->resource();
}

MaaControllerHandle MaaGetController(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return nullptr;
    }
    return inst->controller();
}

MaaBool MaaSyncContextRunTask(MaaSyncContextHandle sync_context, MaaStringView task, MaaStringView param)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(task) << VAR(param);

    if (!sync_context) {
        LogError << "handle is null";
        return false;
    }

    return sync_context->run_task(task, param);
}

void MaaSyncContextClick(MaaSyncContextHandle sync_context, int32_t x, int32_t y)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(x) << VAR(y);

    if (!sync_context) {
        LogError << "handle is null";
        return;
    }

    return sync_context->click(x, y);
}

void MaaSyncContextSwipe(MaaSyncContextHandle sync_context, int32_t* x_steps_buff, int32_t* y_steps_buff,
                         int32_t* step_delay_buff, MaaSize buff_size)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(x_steps_buff) << VAR(y_steps_buff) << VAR(step_delay_buff)
            << VAR(buff_size);

    if (!sync_context) {
        LogError << "handle is null";
        return;
    }

    std::vector<int32_t> x_steps(x_steps_buff, x_steps_buff + buff_size);
    std::vector<int32_t> y_steps(y_steps_buff, y_steps_buff + buff_size);
    std::vector<int32_t> step_delay(step_delay_buff, step_delay_buff + buff_size);

    sync_context->swipe(std::move(x_steps), std::move(y_steps), std::move(step_delay));
}

MaaBool MaaSyncContextScreencap(MaaSyncContextHandle sync_context, MaaImageBufferHandle buffer)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(buffer);

    if (!sync_context || !buffer) {
        LogError << "handle is null";
        return false;
    }

    buffer->set(sync_context->screencap());
    return true;
}

MaaBool MaaSyncContextGetTaskResult(MaaSyncContextHandle sync_context, MaaStringView task, MaaStringBufferHandle buffer)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(buffer);

    if (!sync_context || !buffer) {
        LogError << "handle is null";
        return false;
    }

    buffer->set(sync_context->task_result(task));
    return true;
}

MaaBool MaaSyncContextRunRecognizer(MaaSyncContextHandle sync_context, MaaImageBufferHandle image, MaaStringView task,
                                    MaaStringView task_param, MaaRectHandle box, MaaStringBufferHandle detail_buff)
{
    // TODO
    std::ignore = sync_context;
    std::ignore = image;
    std::ignore = task;
    std::ignore = task_param;

    std::ignore = box;
    std::ignore = detail_buff;

    return false;
}

MaaBool MaaSyncContextRunAction(MaaSyncContextHandle sync_context, MaaStringView task_name, MaaStringView task_param,
                                MaaRectHandle cur_box, MaaStringView cur_rec_detail)
{
    // TODO
    std::ignore = sync_context;
    std::ignore = task_name;
    std::ignore = task_param;
    std::ignore = cur_box;
    std::ignore = cur_rec_detail;

    return false;
}