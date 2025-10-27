#pragma once

#include "Common/MaaTypes.h"
#include "Utils/Dispatcher.hpp"
#include "Utils/Logger.h"
#include "Utils/NonCopyable.hpp"

MAA_NS_BEGIN

struct EventSink
{
    void on_event(void* handle, std::string_view msg, std::string_view detail)
    {
        if (!callback) {
            return;
        }

        callback(handle, msg.data(), detail.data(), trans_arg);
    }

    MaaEventCallback callback = nullptr;
    void* trans_arg = nullptr;
};

class EventDispatcher
    : public IMaaEventDispatcher
    , private Dispatcher<EventSink>
{
public:
    virtual MaaSinkId add_sink(MaaEventCallback callback, void* trans_arg) override
    {
        LogInfo << VAR_VOIDP(callback) << VAR_VOIDP(trans_arg);

        if (!callback) {
            LogWarn << "callback is null";
            return MaaInvalidId;
        }
        return register_observer(std::make_shared<EventSink>(callback, trans_arg));
    }

    virtual void remove_sink(MaaSinkId sink_id) override
    {
        LogInfo << VAR(sink_id);

        unregister_observer(sink_id);
    }

    virtual void clear_sinks() override
    {
        LogInfo;

        clear_observer();
    }

public:
    void notify(void* handle, std::string_view msg, const json::value& details)
    {
        LogFunc << VAR_VOIDP(handle) << VAR(msg) << VAR(details);

        const std::string str_detail = details.to_string();
        dispatch([&](const std::shared_ptr<EventSink>& sink) {
            if (!sink) {
                return;
            }
            sink->on_event(handle, msg, str_detail);
        });
    }
};

MAA_NS_END
