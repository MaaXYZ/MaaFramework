#pragma once

#include "Logger.h"
#include "NonCopyable.hpp"

#include <memory>

MAA_NS_BEGIN

template <typename Callback, typename CallbackArg = MaaCallbackTransparentArg>
    requires std::is_constructible_v<MaaAPICallback, Callback>
class MessageNotifier : public NonCopyable
{
public:
    MessageNotifier(Callback callback, CallbackArg callback_arg) : callback_(callback), callback_arg_(callback_arg) {}
    void notify(std::string_view msg, const json::value& details = json::value())
    {
        LogFunc << VAR_VOIDP(callback_) << VAR_VOIDP(callback_arg_) << VAR(msg) << VAR(details);

        if (!callback_) {
            return;
        }
        callback_(msg.data(), details.to_string().c_str(), callback_arg_);
    }

private:
    Callback callback_ = nullptr;
    CallbackArg callback_arg_ = nullptr;
};

MAA_NS_END
