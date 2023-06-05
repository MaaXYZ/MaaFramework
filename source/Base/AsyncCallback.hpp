#pragma once

#include "Base/AsyncRunner.hpp"
#include "Common/MaaMsg.h"
#include "Utils/Logger.hpp"

#include <memory>

MAA_NS_BEGIN

template <typename Callback, typename CallbackArg = void*>
requires std::is_constructible_v<MaaApiCallback, Callback>
class AsyncCallback
{
private:
    struct NotifyData
    {
        MaaMsg msg = MaaMsg::InvalidMsg;
        json::value details;
    };

public:
    virtual ~AsyncCallback() = default;

protected:
    AsyncCallback(Callback callback, CallbackArg callback_arg) : callback_(callback), callback_arg_(callback_arg)
    {
        notify_runner_ = std::make_unique<AsyncRunner<NotifyData>>(std::bind(
            &AsyncCallback<Callback, CallbackArg>::callback, this, std::placeholders::_1, std::placeholders::_2));
    }

    void notify(MaaMsg msg, json::value details = json::value())
    {
        LogFunc << VAR(notify_runner_) << VAR(msg) << VAR(details);
        notify_runner_->post({ .msg = msg, .details = details });
    }

private:
    void callback(typename AsyncRunner<NotifyData>::Id id, NotifyData cb_data)
    {
        LogFunc << VAR_VOIDP(callback_) << VAR_VOIDP(callback_arg_) << VAR(id) << VAR(cb_data.msg)
                << VAR(cb_data.details);

        if (callback_) {
            callback_(static_cast<MaaMsgId>(id), cb_data.details.to_string().c_str(), callback_arg_);
        }
    }

private:
    Callback callback_ = nullptr;
    CallbackArg callback_arg_ = nullptr;

    std::unique_ptr<AsyncRunner<NotifyData>> notify_runner_ = nullptr;
};

MAA_NS_END
