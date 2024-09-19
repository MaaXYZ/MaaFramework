#pragma once

#include "Utils/Logger.h"
#include "Utils/NonCopyable.hpp"

MAA_NS_BEGIN

class MessageNotifier : public NonCopyable
{
public:
    MessageNotifier(MaaNotificationCallback notify, void* notify_trans_arg)
        : notify_(notify)
        , notify_trans_arg_(notify_trans_arg)
    {
    }

    void notify(std::string_view msg, const json::value& details)
    {
        LogFunc << VAR_VOIDP(notify_) << VAR_VOIDP(notify_trans_arg_) << VAR(msg) << VAR(details);

        if (!notify_) {
            return;
        }

        const std::string str_detail = details.to_string();
        notify_(msg.data(), str_detail.c_str(), notify_trans_arg_);
    }

private:
    MaaNotificationCallback notify_ = nullptr;
    void* notify_trans_arg_ = nullptr;
};

MAA_NS_END
