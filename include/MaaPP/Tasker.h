#pragma once

#include <map>
#include <memory>
#include <tuple>

#include <MaaFramework/MaaAPI.h>

#include "./Exception.h"
#include "./Resource.h"

namespace maapp
{

struct Resource;
struct Controller;

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

    ~Tasker()
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
