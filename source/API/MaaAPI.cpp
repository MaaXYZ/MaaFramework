#include "MaaAPI.h"

#include <meojson/json.hpp>

#include "Controller/AdbController.h"
#include "Controller/CustomController.h"
#include "Controller/MaatouchController.h"
#include "Controller/MinitouchController.h"
#include "Instance/InstanceMgr.h"
#include "Option/GlobalOption.h"
#include "Resource/ResourceMgr.h"
#include "Utils/Logger.hpp"
#include "Utils/Platform.hpp"

static constexpr MaaSize NullSize = static_cast<MaaSize>(-1);

MaaBool MAAAPI MaaSetGlobalOption(MaaString key, MaaString value)
{
    LogFunc << VAR(key) << VAR(value);

    return MAA_NS::GlabalOption::get_instance().set_option(key, value);
}

MaaResourceHandle MaaResourceCreate(MaaString path, MaaString user_path, MaaResourceCallback callback,
                                    void* callback_arg)
{
    LogFunc << VAR(path) << VAR(user_path) << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    return new MAA_RES_NS::ResourceMgr(MAA_NS::path(path), MAA_NS::path(user_path), callback, callback_arg);
}

void MaaResourceDestroy(MaaResourceHandle* res)
{
    LogFunc << VAR_VOIDP(res);

    if (res == nullptr || *res == nullptr) {
        return;
    }
    delete *res;
    *res = nullptr;
}

MaaBool MaaResourceIncrementalLoad(MaaResourceHandle res, MaaString path)
{
    LogFunc << VAR_VOIDP(res) << VAR(path);

    if (!res) {
        return false;
    }
    return res->incremental_load(MAA_NS::path(path));
}

MaaBool MaaResourceSetOption(MaaResourceHandle res, MaaString key, MaaString value)
{
    LogFunc << VAR_VOIDP(res) << VAR(key) << VAR(value);

    if (!res) {
        return false;
    }
    return res->set_option(key, value);
}

MaaBool MaaResourceLoading(MaaResourceHandle res)
{
    // LogFunc << VAR_VOIDP(res);

    if (!res) {
        return false;
    }
    return res->loading();
}

MaaBool MaaResourceLoaded(MaaResourceHandle res)
{
    LogFunc << VAR_VOIDP(res);

    if (!res) {
        return false;
    }
    return res->loaded();
}

MaaSize MaaResourceGetHash(MaaResourceHandle res, char* buff, MaaSize buff_size)
{
    LogFunc << VAR_VOIDP(res) << VAR_VOIDP(buff) << VAR(buff_size);

    if (!res || !buff) {
        return NullSize;
    }
    auto hash = res->get_hash();
    size_t size = hash.size();
    if (size >= buff_size) {
        return NullSize;
    }
    memcpy(buff, hash.c_str(), size);
    return size;
}

MaaControllerHandle MaaAdbControllerCreate(MaaString adb_path, MaaString address, MaaString config_json,
                                           MaaControllerCallback callback, void* callback_arg)
{
    LogFunc << VAR(adb_path) << VAR(address) << VAR(config_json) << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    auto config_opt = MAA_CTRL_NS::AdbConfig::parse(config_json);
    if (!config_opt) {
        return nullptr;
    }

    return new MAA_CTRL_NS::AdbController(adb_path, address, *config_opt, callback, callback_arg);
}

MaaControllerHandle MaaMinitouchControllerCreate(MaaString adb_path, MaaString address, MaaString config_json,
                                                 MaaControllerCallback callback, void* callback_arg)
{
    LogFunc << VAR(adb_path) << VAR(address) << VAR(config_json) << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    auto config_opt = MAA_CTRL_NS::MinitouchConfig::parse(config_json);
    if (!config_opt) {
        return nullptr;
    }

    return new MAA_CTRL_NS::MinitouchController(adb_path, address, *config_opt, callback, callback_arg);
}

MaaControllerHandle MaaMaatouchControllerCreate(MaaString adb_path, MaaString address, MaaString config_json,
                                                MaaControllerCallback callback, void* callback_arg)
{
    LogFunc << VAR(adb_path) << VAR(address) << VAR(config_json) << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    auto config_opt = MAA_CTRL_NS::MaatouchConfig::parse(config_json);
    if (!config_opt) {
        return nullptr;
    }

    return new MAA_CTRL_NS::MaatouchController(adb_path, address, *config_opt, callback, callback_arg);
}

MaaControllerHandle MaaCustomControllerCreate(MaaCustomControllerHandle handle, MaaControllerCallback callback,
                                              void* callback_arg)
{
    LogFunc << VAR(handle) << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    if (!handle) {
        return nullptr;
    }

    return new MAA_CTRL_NS::CustomController(handle, callback, callback_arg);
}

void MaaControllerDestroy(MaaControllerHandle* ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (ctrl == nullptr || *ctrl == nullptr) {
        return;
    }
    delete *ctrl;
    *ctrl = nullptr;
}

MaaBool MaaControllerSetOption(MaaControllerHandle ctrl, MaaString key, MaaString value)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(key) << VAR(value);

    if (!ctrl) {
        return false;
    }
    return ctrl->set_option(key, value);
}

MaaBool MaaControllerConnecting(MaaControllerHandle ctrl)
{
    // LogFunc << VAR_VOIDP(ctrl);

    if (!ctrl) {
        return false;
    }
    return ctrl->connecting();
}

MaaBool MaaControllerConnected(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (!ctrl) {
        return false;
    }
    return ctrl->connected();
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
    return ctrl->post_swipe(x_steps, y_steps, step_delay);
}

MaaCtrlId MaaControllerPostScreencap(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (!ctrl) {
        return MaaInvalidId;
    }
    return ctrl->post_screencap();
}

MaaSize MaaControllerGetImage(MaaControllerHandle ctrl, void* buff, MaaSize buff_size)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR_VOIDP(buff) << VAR(buff_size);

    if (!ctrl || !buff) {
        return NullSize;
    }
    auto image = ctrl->get_image();
    size_t size = image.size();
    if (size >= buff_size) {
        return NullSize;
    }
    memcpy(buff, image.data(), size);
    return size;
}

MaaSize MaaControllerGetUUID(MaaControllerHandle ctrl, char* buff, MaaSize buff_size)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR_VOIDP(buff) << VAR(buff_size);

    if (!ctrl || !buff) {
        return NullSize;
    }
    auto uuid = ctrl->get_uuid();
    size_t size = uuid.size();
    if (size >= buff_size) {
        return NullSize;
    }
    memcpy(buff, uuid.c_str(), size);
    return size;
}

MaaInstanceHandle MaaCreate(MaaInstanceCallback callback, void* callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    return new MAA_NS::InstanceMgr(callback, callback_arg);
}

void MaaDestroy(MaaInstanceHandle* inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (inst == nullptr || *inst == nullptr) {
        return;
    }
    delete *inst;
    *inst = nullptr;
}

MaaBool MaaSetOption(MaaInstanceHandle inst, MaaString key, MaaString value)
{
    LogFunc << VAR_VOIDP(inst) << VAR(key) << VAR(value);

    if (!inst) {
        return false;
    }
    return inst->set_option(key, value);
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
    LogFunc << VAR_VOIDP(inst);
    if (!inst) {
        return false;
    }
    return inst->inited();
}

MaaTaskId MaaPostTask(MaaInstanceHandle inst, MaaString type, MaaString param)
{
    LogFunc << VAR_VOIDP(inst) << VAR(type) << VAR(param);

    if (!inst) {
        return MaaInvalidId;
    }
    return inst->post_task(type, param);
}

MaaBool MaaSetTaskParam(MaaInstanceHandle inst, MaaTaskId id, MaaString param)
{
    LogFunc << VAR_VOIDP(inst) << VAR(id) << VAR(param);

    if (!inst) {
        return false;
    }
    return inst->set_task_param(id, param);
}

void MaaStop(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        return;
    }

    inst->stop();
}

MaaBool MaaRunning(MaaInstanceHandle inst)
{
    // LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        return false;
    }
    return inst->running();
}

MaaSize MaaGetResourceHash(MaaInstanceHandle inst, char* buff, MaaSize buff_size)
{
    LogFunc << VAR_VOIDP(inst) << VAR_VOIDP(buff) << VAR(buff_size);

    if (!inst || !buff) {
        return NullSize;
    }
    auto hash = inst->get_resource_hash();
    size_t size = hash.size();
    if (size >= buff_size) {
        return NullSize;
    }
    memcpy(buff, hash.c_str(), size);
    return size;
}

MaaSize MaaGetControllerUUID(MaaInstanceHandle inst, char* buff, MaaSize buff_size)
{
    LogFunc << VAR_VOIDP(inst) << VAR_VOIDP(buff) << VAR(buff_size);

    if (!inst || !buff) {
        return NullSize;
    }
    auto uuid = inst->get_controller_uuid();
    size_t size = uuid.size();
    if (size >= buff_size) {
        return NullSize;
    }
    memcpy(buff, uuid.c_str(), size);
    return size;
}

MaaSize MaaGetTaskList(MaaInstanceHandle inst, MaaTaskId* buff, MaaSize buff_size)
{
    LogFunc << VAR_VOIDP(inst) << VAR_VOIDP(buff) << VAR(buff_size);

    if (!inst || !buff) {
        return NullSize;
    }
    auto tasks = inst->get_task_list();
    size_t size = tasks.size();
    if (size >= buff_size) {
        return NullSize;
    }
    memcpy(buff, tasks.data(), size * sizeof(MaaTaskId));
    return size;
}

MaaString MaaVersion()
{
    return MAA_VERSION;
}
