#pragma once

#include <memory>
#include <optional>
#include <utility>

#include "MaaFramework/Instance/MaaResource.h"
#include "MaaFramework/MaaAPI.h"
#include "MaaFramework/MaaDef.h"
#include "MaaFramework/MaaMsg.h"
#include "MaaPP/details/Callback.hpp"
#include "MaaPP/details/utils/FutureAwaiter.hpp"
#include "MaaPP/details/utils/Task.hpp"

namespace maa
{

class Resource
{
public:
    struct ResourceAction
    {
        Resource& res_;
        MaaResId id_;
        std::promise<MaaStatus> status_;
        std::future<MaaStatus> status_future_;

        ResourceAction(Resource& res, MaaResId id)
            : res_(res)
            , id_(id)
            , status_future_(status_.get_future())
        {
        }

        ResourceAction(const ResourceAction&) = delete;
        ResourceAction& operator=(const ResourceAction&) = delete;

        ~ResourceAction() { res_.actions_.erase(id_); }

        MaaStatus status() { return MaaResourceStatus(res_.res_, id_); }

        utils::Task<MaaStatus> wait()
        {
            std::cout << "start wait status" << std::endl;
            co_await utils::FutureAwaiter<MaaStatus>(status_future_);
            co_return status_future_.get();
            // while (true) {
            //     if (co_await utils::FutureAwaiter<MaaStatus>(status_future_)) {
            //         co_return status_future_.get();
            //     }
            // }
        }
    };

    Resource()
        : cb_pull_(process_callback())
    {
        res_ = MaaResourceCreate(cb_.func_ptr(), cb_.callback_arg());
    }

    Resource(const Resource&) = delete;

    Resource& operator=(const Resource&) = delete;

    ~Resource()
    {
        std::cout << "resource destroy" << std::endl;
        if (res_) {
            MaaResourceDestroy(res_);
        }
    }

    std::shared_ptr<ResourceAction> post_path(const std::string& path)
    {
        auto id = MaaResourcePostPath(res_, path.c_str());
        if (id == MaaInvalidId) {
            return nullptr;
        }
        auto ret = std::make_shared<ResourceAction>(*this, id);
        actions_[id] = ret;
        return ret;
    }

private:
    MaaResourceHandle res_ = nullptr;
    Callback cb_;
    std::map<MaaResId, std::shared_ptr<ResourceAction>> actions_;
    utils::Task<void> cb_pull_;

    utils::Task<void> process_callback()
    {
        while (true) {
            try {
                auto value = co_await cb_;
                std::cout << value.msg << ' ' << value.details.to_string() << std::endl;
                if (!value.details.is_object()) {
                    continue;
                }
                const auto& obj = value.details.as_object();
                if (!obj.contains("id")) {
                    continue;
                }
                if (!obj.at("id").is_number()) {
                    continue;
                }
                MaaResId id = obj.at("id").as_unsigned_long_long();
                if (!actions_.contains(id)) {
                    continue;
                }
                auto ptr = actions_[id];
                if (value.msg == MaaMsg_Resource_StartLoading) {
                }
                else if (value.msg == MaaMsg_Resource_LoadingCompleted) {
                    ptr->status_.set_value(MaaStatus_Success);
                }
                else if (value.msg == MaaMsg_Resource_LoadingFailed) {
                    ptr->status_.set_value(MaaStatus_Failed);
                }
            }
            catch (const std::runtime_error&) {
                break;
            }
        }
    }
};

}
