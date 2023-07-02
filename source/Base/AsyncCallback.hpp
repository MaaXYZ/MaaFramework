#pragma once

#include "Base/AsyncRunner.hpp"
#include "MaaMsg.h"
#include "MaaUtils/Logger.hpp"

#include <memory>

MAA_NS_BEGIN

template <typename Callback, typename CallbackArg = void*>
requires std::is_constructible_v<MaaAPICallback, Callback>
class AsyncCallback : public NonCopyable
{
private:
    struct NotifyData
    {
        MaaMsg msg = MaaMsg_Invalid;
        json::value details;
    };

public:
    AsyncCallback(Callback callback, CallbackArg callback_arg) : callback_(callback), callback_arg_(callback_arg)
    {
        notify_runner_ = std::make_unique<AsyncRunner<NotifyData>>(std::bind(
            &AsyncCallback<Callback, CallbackArg>::callback, this, std::placeholders::_1, std::placeholders::_2));
    }

    virtual ~AsyncCallback() { release(); }

    void release()
    {
        if (notify_runner_) {
            notify_runner_->release();
        }
    }

    void notify(MaaMsg msg, json::value details = json::value())
    {
        LogTrace << VAR(notify_runner_) << VAR(msg) << VAR(details);
        if (!notify_runner_) {
            return;
        }
        notify_runner_->post({ .msg = msg, .details = std::move(details) });
    }

private:
    bool callback(typename AsyncRunner<NotifyData>::Id id, NotifyData cb_data)
    {
        // LogFunc << VAR_VOIDP(callback_) << VAR_VOIDP(callback_arg_) << VAR(id) << VAR(cb_data.msg)
        //         << VAR(cb_data.details);

        std::ignore = id;

        if (!callback_) {
            return false;
        }

        callback_(cb_data.msg, cb_data.details.to_string().c_str(), callback_arg_);
        return true;
    }

private:
    Callback callback_ = nullptr;
    CallbackArg callback_arg_ = nullptr;

    std::unique_ptr<AsyncRunner<NotifyData>> notify_runner_ = nullptr;
};

MAA_NS_END
