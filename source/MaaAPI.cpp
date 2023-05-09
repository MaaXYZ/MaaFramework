#include "MaaAPI.h"

#include <meojson/json.hpp>

#include "Controller/AdbController.h"
#include "Controller/MaatouchController.h"
#include "Controller/MacPlayToolsController.h"
#include "Controller/MinitouchController.h"
#include "Instance/InstanceMgr.h"
#include "Resource/ResourceMgr.h"
#include "Utils/Platform.hpp"

static constexpr MaaSize NullSize = static_cast<MaaSize>(-1);
static constexpr MaaId InvalidId = 0;

MaaResourceHandle MaaResourceCreate(const char* path, const char* user_path, MaaResourceCallback callback,
                                      void* callback_arg)
{
    return new MAA_RES_NS::ResourceMgr(MAA_NS::path(path), MAA_NS::path(user_path), callback, callback_arg);
}

void MaaResourceDestroy(MaaResourceHandle* res)
{
    if (res == nullptr || *res == nullptr) {
        return;
    }
    delete *res;
    *res = nullptr;
}

MaaBool MaaResourceIncrementalLoad(MaaResourceHandle res, const char* path)
{
    if (!res) {
        return false;
    }
    return res->incremental_load(MAA_NS::path(path));
}

MaaBool MaaResourceSetOption(MaaResourceHandle res, MaaResourceOptionKey key, const char* value)
{
    if (!res) {
        return false;
    }
    return res->set_option(static_cast<MAA_NS::ResourceOptionKey>(key), value);
}

MaaBool MaaResourceLoading(MaaResourceHandle res)
{
    if (!res) {
        return false;
    }
    return res->loading();
}

MaaBool MaaResourceLoaded(MaaResourceHandle res)
{
    if (!res) {
        return false;
    }
    return res->loaded();
}

MaaSize MaaResourceGetHash(MaaResourceHandle res, char* buff, MaaSize buff_size)
{
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

MaaControllerHandle MaaAdbControllerCreate(const char* adb_path, const char* address, const char* config_json,
                                             MaaControllerCallback callback, void* callback_arg)
{
    auto config_opt = MAA_CTRL_NS::AdbConfig::parse(config_json);
    if (!config_opt) {
        return nullptr;
    }

    return new MAA_CTRL_NS::AdbController(adb_path, address, *config_opt, callback, callback_arg);
}

MaaControllerHandle MaaMinitouchControllerCreate(const char* adb_path, const char* address, const char* config_json,
                                                   MaaControllerCallback callback, void* callback_arg)
{
    auto config_opt = MAA_CTRL_NS::MinitouchConfig::parse(config_json);
    if (!config_opt) {
        return nullptr;
    }

    return new MAA_CTRL_NS::MinitouchController(adb_path, address, *config_opt, callback, callback_arg);
}

MaaControllerHandle MaaMaatouchControllerCreate(const char* adb_path, const char* address, const char* config_json,
                                                  MaaControllerCallback callback, void* callback_arg)
{
    auto config_opt = MAA_CTRL_NS::MaatouchConfig::parse(config_json);
    if (!config_opt) {
        return nullptr;
    }

    return new MAA_CTRL_NS::MaatouchController(adb_path, address, *config_opt, callback, callback_arg);
}

MaaControllerHandle MaaMacPlayToolsControllerCreate(const char* config_json, MaaControllerCallback callback,
                                                      void* callback_arg)
{
    auto config_opt = MAA_CTRL_NS::MacPlayToolsControllerConfig::parse(config_json);
    if (!config_opt) {
        return nullptr;
    }

    return new MAA_CTRL_NS::MacPlayToolsController(*config_opt, callback, callback_arg);
}

void MaaControllerDestroy(MaaControllerHandle* ctrl)
{
    if (ctrl == nullptr || *ctrl == nullptr) {
        return;
    }
    delete *ctrl;
    *ctrl = nullptr;
}

MaaBool MaaControllerSetOption(MaaControllerHandle ctrl, MaaControllerOptionKey key, const char* value)
{
    if (!ctrl) {
        return false;
    }
    return ctrl->set_option(static_cast<MAA_NS::ControllerOptionKey>(key), value);
}

MaaBool MaaControllerConnecting(MaaControllerHandle ctrl)
{
    if (!ctrl) {
        return false;
    }
    return ctrl->connecting();
}

MaaBool MaaControllerConnected(MaaControllerHandle ctrl)
{
    if (!ctrl) {
        return false;
    }
    return ctrl->connected();
}

MaaCtrlId MaaControllerClick(MaaControllerHandle ctrl, int32_t x, int32_t y)
{
    if (!ctrl) {
        return InvalidId;
    }
    return ctrl->click(x, y);
}

MaaCtrlId MaaControllerSwipe(MaaControllerHandle ctrl, int32_t* x_steps_buff, int32_t* y_steps_buff,
                             int32_t* step_delay_buff, MaaSize buff_size)
{
    if (!ctrl || !x_steps_buff || !y_steps_buff || !step_delay_buff || buff_size < 2) {
        return InvalidId;
    }
    std::vector<int32_t> x_steps(x_steps_buff, x_steps_buff + buff_size);
    std::vector<int32_t> y_steps(y_steps_buff, y_steps_buff + buff_size);
    std::vector<int32_t> step_delay(step_delay_buff, step_delay_buff + buff_size);
    return ctrl->swipe(x_steps, y_steps, step_delay);
}

MaaCtrlId MaaControllerScreencap(MaaControllerHandle ctrl)
{
    if (!ctrl) {
        return InvalidId;
    }
    return ctrl->screencap();
}

MaaSize MaaControllerGetImage(MaaControllerHandle ctrl, void* buff, MaaSize buff_size)
{
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
    return new MAA_NS::InstanceMgr(callback, callback_arg);
}

void MaaDestroy(MaaInstanceHandle* inst)
{
    if (inst == nullptr || *inst == nullptr) {
        return;
    }
    delete *inst;
    *inst = nullptr;
}

MaaBool MaaSetOption(MaaInstanceHandle inst, MaaInstanceOptionKey key, const char* value)
{
    if (!inst) {
        return false;
    }
    return inst->set_option(static_cast<MAA_NS::InstanceOptionKey>(key), value);
}

MaaBool MaaBindResource(MaaInstanceHandle inst, MaaResourceHandle res)
{
    if (!inst || !res) {
        return false;
    }
    return inst->bind_resource(res);
}

MaaBool MaaBindController(MaaInstanceHandle inst, MaaControllerHandle ctrl)
{
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

MaaTaskId MaaAppendTask(MaaInstanceHandle inst, const char* type, const char* param)
{
    if (!inst) {
        return InvalidId;
    }
    return inst->append_task(type, param);
}

MaaBool MaaSetTaskParam(MaaInstanceHandle inst, MaaTaskId id, const char* param)
{
    if (!inst) {
        return false;
    }
    return inst->set_task_param(id, param);
}

MaaBool MaaStart(MaaInstanceHandle inst)
{
    if (!inst) {
        return false;
    }
    return inst->start();
}

MaaBool MaaStop(MaaInstanceHandle inst)
{
    if (!inst) {
        return false;
    }
    return inst->stop();
}

MaaBool MaaRunning(MaaInstanceHandle inst)
{
    if (!inst) {
        return false;
    }
    return inst->running();
}

MaaSize MaaGetResourceHash(MaaInstanceHandle inst, char* buff, MaaSize buff_size)
{
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

const char* MaaVersion()
{
    return MAA_VERSION;
}
