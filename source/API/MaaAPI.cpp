#include "MaaAPI.h"

#include <meojson/json.hpp>

#include "Controller/AdbController.h"
#include "Controller/CustomController.h"
#include "Instance/InstanceMgr.h"
#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaUtils/Logger.hpp"
#include "Option/GlobalOptionMgr.h"
#include "Resource/ResourceMgr.h"
#include "Utils/Platform.hpp"

MaaBool MaaSetGlobalOption(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    return MAA_NS::GlobalOptionMgr::get_instance().set_option(key, value, val_size);
}

MaaResourceHandle MaaResourceCreate(MaaString user_path, MaaResourceCallback callback,
                                    MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR(user_path) << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    return new MAA_RES_NS::ResourceMgr(MAA_NS::path(user_path), callback, callback_arg);
}

void MaaResourceDestroy(MaaResourceHandle res)
{
    LogFunc << VAR_VOIDP(res);

    if (res == nullptr) {
        return;
    }
    delete res;
}

MaaResId MaaResourcePostResource(MaaResourceHandle res, MaaString path)
{
    LogFunc << VAR_VOIDP(res) << VAR(path);

    if (!res) {
        return MaaInvalidId;
    }
    return res->post_resource(MAA_NS::path(path));
}

MaaStatus MaaResourceStatus(MaaResourceHandle res, MaaResId id)
{
    // LogFunc << VAR_VOIDP(res) << VAR(id);

    if (!res) {
        return MaaStatus_Invalid;
    }
    return res->status(id);
}

MaaStatus MaaResourceWait(MaaResourceHandle res, MaaResId id)
{
    // LogFunc << VAR_VOIDP(res) << VAR(id);

    if (!res) {
        return MaaStatus_Invalid;
    }
    return res->wait(id);
}

MaaBool MaaResourceLoaded(MaaResourceHandle res)
{
    // LogFunc << VAR_VOIDP(res);

    if (!res) {
        return false;
    }
    return res->loaded();
}

MaaBool MaaResourceSetOption(MaaResourceHandle res, MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(res) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!res) {
        return false;
    }
    return res->set_option(key, value, val_size);
}

MaaSize MaaResourceGetHash(MaaResourceHandle res, char* buff, MaaSize buff_size)
{
    LogFunc << VAR_VOIDP(res) << VAR_VOIDP(buff) << VAR(buff_size);

    if (!res || !buff) {
        return MaaNullSize;
    }
    auto hash = res->get_hash();
    size_t size = hash.size();
    if (size >= buff_size) {
        return MaaNullSize;
    }
    memcpy(buff, hash.c_str(), size);
    return size;
}

MaaControllerHandle MaaAdbControllerCreate(MaaString adb_path, MaaString address, MaaAdbControllerType type,
                                           MaaJsonString config, MaaControllerCallback callback,
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
        return nullptr;
    }

    return new MAA_CTRL_NS::CustomController(handle, callback, callback_arg);
}

void MaaControllerDestroy(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (ctrl == nullptr) {
        return;
    }
    delete ctrl;
}

MaaBool MaaControllerSetOption(MaaControllerHandle ctrl, MaaCtrlOption key, MaaOptionValue value,
                               MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!ctrl) {
        return false;
    }
    return ctrl->set_option(key, value, val_size);
}

MaaCtrlId MaaControllerPostConnection(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);
    if (!ctrl) {
        return false;
    }
    return ctrl->post_connection();
}

MaaCtrlId MaaControllerPostClick(MaaControllerHandle ctrl, int32_t x, int32_t y)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(x) << VAR(y);

    if (!ctrl) {
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
        return MaaInvalidId;
    }
    return ctrl->post_screencap();
}

MaaStatus MaaControllerStatus(MaaControllerHandle ctrl, MaaCtrlId id)
{
    // LogFunc << VAR_VOIDP(ctrl) << VAR(id);

    if (!ctrl) {
        return MaaStatus_Invalid;
    }
    return ctrl->status(id);
}

MaaStatus MaaControllerWait(MaaControllerHandle ctrl, MaaCtrlId id)
{
    // LogFunc << VAR_VOIDP(ctrl) << VAR(id);

    if (!ctrl) {
        return MaaStatus_Invalid;
    }
    return ctrl->wait(id);
}

MaaBool MaaControllerConnected(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);
    if (!ctrl) {
        return false;
    }
    return ctrl->connected();
}

MaaSize MaaControllerGetImage(MaaControllerHandle ctrl, void* buff, MaaSize buff_size)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR_VOIDP(buff) << VAR(buff_size);

    if (!ctrl || !buff) {
        return MaaNullSize;
    }
    auto image = ctrl->get_image_cache();
    size_t size = image.size();
    if (size >= buff_size) {
        return MaaNullSize;
    }
    memcpy(buff, image.data(), size);
    return size;
}

MaaSize MaaControllerGetUUID(MaaControllerHandle ctrl, char* buff, MaaSize buff_size)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR_VOIDP(buff) << VAR(buff_size);

    if (!ctrl || !buff) {
        return MaaNullSize;
    }
    auto uuid = ctrl->get_uuid();
    size_t size = uuid.size();
    if (size >= buff_size) {
        return MaaNullSize;
    }
    memcpy(buff, uuid.c_str(), size);
    return size;
}

MaaInstanceHandle MaaCreate(MaaInstanceCallback callback, MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    return new MAA_NS::InstanceMgr(callback, callback_arg);
}

void MaaDestroy(MaaInstanceHandle* inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (inst == nullptr) {
        return;
    }
    delete inst;
}

MaaBool MaaSetOption(MaaInstanceHandle inst, MaaInstOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(inst) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!inst) {
        return false;
    }
    return inst->set_option(key, value, val_size);
}

MaaBool MaaBindResource(MaaInstanceHandle inst, MaaResourceHandle res)
{
    LogFunc << VAR_VOIDP(inst) << VAR_VOIDP(res);

    if (!inst || !res) {
        return false;
    }
    return inst->bind_resource(res);
}

MaaBool MaaBindController(MaaInstanceHandle inst, MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(inst) << VAR_VOIDP(ctrl);

    if (!inst || !ctrl) {
        return false;
    }
    return inst->bind_controller(ctrl);
}

MaaBool MaaInited(MaaInstanceHandle inst)
{
    if (!inst) {
        return false;
    }
    return inst->inited();
}

void MaaRegisterCustomTask(MaaInstanceHandle inst, MaaString name, MaaCustomTaskHandle task)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name) << VAR_VOIDP(task);

    if (!inst) {
        return;
    }

    inst->register_custom_task(name, task);
}

void MaaUnregisterCustomTask(MaaInstanceHandle inst, MaaString name)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name);

    if (!inst) {
        return;
    }
    inst->unregister_custom_task(name);
}

void MaaClearCustomTask(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        return;
    }

    inst->clear_custom_task();
}

MaaTaskId MaaPostTask(MaaInstanceHandle inst, MaaString task, MaaJsonString param)
{
    LogFunc << VAR_VOIDP(inst) << VAR(task) << VAR(param);

    if (!inst) {
        return MaaInvalidId;
    }
    return inst->post_task(task, param);
}

MaaBool MaaSetTaskParam(MaaInstanceHandle inst, MaaTaskId id, MaaJsonString param)
{
    LogFunc << VAR_VOIDP(inst) << VAR(id) << VAR(param);

    if (!inst) {
        return false;
    }
    return inst->set_task_param(id, param);
}

MaaStatus MaaTaskStatus(MaaInstanceHandle inst, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(inst) << VAR(id);

    if (!inst) {
        return MaaStatus_Invalid;
    }
    return inst->status(id);
}

MaaStatus MaaTaskWait(MaaInstanceHandle inst, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(inst) << VAR(id);

    if (!inst) {
        return MaaStatus_Invalid;
    }
    return inst->wait(id);
}

MaaBool MaaTaskAllFinished(MaaInstanceHandle inst)
{
    // LogFunc << VAR_VOIDP(inst) << VAR(id);
    if (!inst) {
        return false;
    }
    return inst->all_finished();
}

void MaaStop(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        return;
    }

    inst->stop();
}

MaaResourceHandle MAA_API MaaGetResource(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        return nullptr;
    }

    return inst->resource();
}

MaaControllerHandle MAA_API MaaGetController(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);
    if (!inst) {
        return nullptr;
    }
    return inst->controller();
}

MaaString MaaVersion()
{
    return MAA_VERSION;
}
