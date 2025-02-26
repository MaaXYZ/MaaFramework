#pragma once

#include <map>
#include <memory>
#include <tuple>

#include <MaaFramework/MaaAPI.h>

#include "./Controller.h"
#include "./Exception.h"
#include "./Resource.h"
#include "Task.h"

namespace maapp
{

struct Tasker : public std::enable_shared_from_this<Tasker>
{
    Tasker()
    {
        tasker_ = MaaTaskerCreate(
            +[](const char* message, const char* details_json, void* notify_trans_arg) {
                auto self = static_cast<Tasker*>(notify_trans_arg)->shared_from_this();

                std::ignore = message;
                std::ignore = details_json;
            },
            this);

        tasker_map_[tasker_] = weak_from_this();
    }

    virtual ~Tasker()
    {
        tasker_map_.erase(tasker_);
        MaaTaskerDestroy(tasker_);
    }

    void bind(std::shared_ptr<Resource> resource)
    {
        if (!MaaTaskerBindResource(tasker_, resource->resource_)) {
            throw FunctionFailed("MaaTaskerBindResource");
        }
        resource_ = resource;
    }

    void bind(std::shared_ptr<Controller> controller)
    {
        if (!MaaTaskerBindController(tasker_, controller->controller_)) {
            throw FunctionFailed("MaaTaskerBindController");
        }
        controller_ = controller;
    }

    bool inited() const { return MaaTaskerInited(tasker_); }

    Task post_task(const std::string& entry, const std::string& pipeline_override)
    {
        return {
            tasker_,
            MaaTaskerPostTask(tasker_, entry.c_str(), pipeline_override.c_str()),
        };
    }

    bool running() const { return MaaTaskerRunning(tasker_); }

    Task post_stop()
    {
        return {
            tasker_,
            MaaTaskerPostStop(tasker_),
        };
    }

    std::shared_ptr<Resource> resource() const { return resource_; }

    std::shared_ptr<Controller> controller() const { return controller_; }

    void clear_cache() const
    {
        if (!MaaTaskerClearCache(tasker_)) {
            throw FunctionFailed("MaaTaskerClearCache");
        }
    }

    MaaTasker* tasker_ {};
    std::shared_ptr<Resource> resource_ {};
    std::shared_ptr<Controller> controller_ {};

    static std::shared_ptr<Tasker> find(MaaTasker* tasker)
    {
        auto it = tasker_map_.find(tasker);
        if (it == tasker_map_.end()) {
            return nullptr;
        }
        return it->second.lock();
    }

    static inline std::map<MaaTasker*, std::weak_ptr<Tasker>> tasker_map_ {};
};

}
