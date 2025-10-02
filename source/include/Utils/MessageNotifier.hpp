#pragma once

#include "Common/MaaTypes.h"
#include "Utils/Dispatcher.hpp"
#include "Utils/Logger.h"
#include "Utils/NonCopyable.hpp"

MAA_NS_BEGIN

struct MessageSink
{
    void on_message(std::string_view msg, std::string_view detail)
    {
        if (!callback) {
            return;
        }

        callback(msg.data(), detail.data(), trans_arg);
    }

    MaaNotificationCallback callback = nullptr;
    void* trans_arg = nullptr;
};

class MessageNotifier
    : public NonCopyable
    , public IMaaNotifier
    , private Dispatcher<MessageSink>
{
public:
    virtual void add_sink(MaaNotificationCallback callback, void* trans_arg) override
    {
        LogInfo << VAR_VOIDP(callback) << VAR_VOIDP(trans_arg);

        if (!callback) {
            LogWarn << "callback is null";
            return;
        }

        auto id = register_observer(std::make_shared<MessageSink>(callback, trans_arg));
        observer_id_map_.emplace(id, callback);
    }

    virtual void remove_sink(MaaNotificationCallback callback) override
    {
        LogInfo << VAR_VOIDP(callback);

        if (!callback) {
            LogWarn << "callback is null";
            return;
        }

        auto it = std::ranges::find_if(observer_id_map_, [&](const auto& pair) { return pair.second == callback; });
        if (it == observer_id_map_.end()) {
            return;
        }

        unregister_observer(it->first);
        observer_id_map_.erase(it);
    }

    virtual void clear_sinks() override
    {
        LogInfo;

        clear_observer();
    }

public:
    void notify(std::string_view msg, const json::value& details)
    {
        LogFunc << VAR(msg) << VAR(details);

        const std::string str_detail = details.to_string();
        dispatch([&](const std::shared_ptr<MessageSink>& sink) {
            if (!sink) {
                return;
            }
            sink->on_message(msg, str_detail);
        });
    }

private:
    std::map<ObserverId, MaaNotificationCallback> observer_id_map_;
};

MAA_NS_END
