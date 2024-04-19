#pragma once

#include <iostream>
#include <meojson/json.hpp>

#include "MaaFramework/MaaAPI.h"
#include "MaaPP/details/utils/Generator.hpp"

namespace maa
{

class Callback
{
public:
    struct CallbackData
    {
        std::string msg;
        json::value details;
    };

    Callback()
    {
        func_ptr_ =
            [](MaaStringView msg, MaaStringView details_json, MaaTransparentArg callback_arg) {
                std::cout << "push" << std::endl;
                Callback* cb = reinterpret_cast<Callback*>(callback_arg);
                cb->cached_generator_.push(
                    CallbackData { msg, json::parse(details_json).value_or(json::object {}) });
            };
    }

    ~Callback() { std::cout << "callback destroy" << std::endl; }

    auto operator co_await() { return cached_generator_.generator_.operator co_await(); }

    MaaAPICallback func_ptr() const { return func_ptr_; }

    void* callback_arg() { return reinterpret_cast<void*>(this); }

private:
    utils::CachedGenerator<Callback::CallbackData> cached_generator_;
    MaaAPICallback func_ptr_ = nullptr;
};

}
