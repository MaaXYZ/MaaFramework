#pragma once

#include <MaaFramework/MaaAPI.h>
#include <iostream>
#include <napi.h>

#include "utils.h"
#include "wrapper.h"

inline void NotificationCallback(const char* message, const char* details_json, void* callback_arg)
{
    auto ctx = reinterpret_cast<CallbackContext*>(callback_arg);
    ctx->Call<void>(
        [=](auto env, auto fn) { return fn.Call({ Napi::String::New(env, message), Napi::String::New(env, details_json) }); },
        [](auto res) { std::ignore = res; });
}

inline MaaBool CustomRecognizerCallback(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_recognition_name,
    const char* custom_recognition_param,
    const MaaImageBuffer* image,
    const MaaRect* roi,
    void* trans_arg,
    MaaRect* out_box,
    MaaStringBuffer* out_detail)
{
    auto ctx = reinterpret_cast<CallbackContext*>(trans_arg);
    using Result = std::optional<std::tuple<MaaRect, std::string>>;
    auto res = ctx->Call<Result>(
        [=](auto env, auto fn) {
            return fn.Call(
                {
                    Napi::External<MaaContext>::New(env, context),
                    JSConvert<MaaTaskId>::to_value(env, task_id),
                    Napi::String::New(env, node_name),
                    Napi::String::New(env, custom_recognition_name),
                    Napi::String::New(env, custom_recognition_param),
                    ImageBufferRefer(image).data(env),
                    JSConvert<MaaRect>::to_value(env, *roi),
                });
        },
        [](Napi::Value res) -> Result {
            try {
                return JSConvert<Result>::from_value(res);
            }
            catch (const MaaNodeException& exc) {
                std::cerr << exc.what() << std::endl;
                return std::nullopt;
            }
        });

    if (res.has_value()) {
        *out_box = std::get<0>(res.value());
        StringBuffer(out_detail, false).set(std::get<1>(res.value()));
        return true;
    }
    else {
        return false;
    }
}

inline MaaBool CustomActionCallback(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_action_name,
    const char* custom_action_param,
    MaaRecoId reco_id,
    const MaaRect* box,
    void* trans_arg)
{
    auto ctx = reinterpret_cast<CallbackContext*>(trans_arg);

    auto res = ctx->Call<bool>(
        [=](auto env, auto fn) {
            return fn.Call(
                {
                    Napi::External<MaaContext>::New(env, context),
                    JSConvert<MaaTaskId>::to_value(env, task_id),
                    Napi::String::New(env, node_name),
                    Napi::String::New(env, custom_action_name),
                    Napi::String::New(env, custom_action_param),
                    JSConvert<MaaRecoId>::to_value(env, reco_id),
                    JSConvert<MaaRect>::to_value(env, *box),
                });
        },
        [](Napi::Value res) -> bool {
            try {
                return JSConvert<bool>::from_value(res);
            }
            catch (const MaaNodeException& exc) {
                std::cerr << exc.what() << std::endl;
                return false;
            }
        });

    return res;
}

inline MaaBool CustomControllerConnect(void* trans_arg)
{
    auto ctx = reinterpret_cast<CallbackContext*>(trans_arg);

    auto res = ctx->Call<bool>(
        [](auto env, auto fn) { return fn.Call({ Napi::String::New(env, "connect") }); },
        [](Napi::Value res) {
            try {
                return JSConvert<bool>::from_value(res);
            }
            catch (const MaaNodeException& exc) {
                std::cerr << exc.what() << std::endl;
                return false;
            }
        });

    return res;
}

inline MaaBool CustomControllerRequestUUID(void* trans_arg, MaaStringBuffer* buffer)
{
    auto ctx = reinterpret_cast<CallbackContext*>(trans_arg);
    using R = std::optional<std::string>;
    auto res = ctx->Call<R>(
        [](auto env, auto fn) { return fn.Call({ Napi::String::New(env, "request_uuid") }); },
        [=](Napi::Value res) -> R {
            if (res.IsNull()) {
                return std::nullopt;
            }
            else {
                auto obj = res.As<Napi::String>();
                return R::value_type { obj.Utf8Value() };
            }
        });

    if (res.has_value()) {
        StringBuffer buf(buffer, false);
        buf.set(res.value());
        return true;
    }
    else {
        return false;
    }
}

inline MaaBool CustomControllerStartApp(const char* intent, void* trans_arg)
{
    auto ctx = reinterpret_cast<CallbackContext*>(trans_arg);

    auto res = ctx->Call<bool>(
        [=](auto env, auto fn) { return fn.Call({ Napi::String::New(env, "start_app"), Napi::String::New(env, intent) }); },
        [](Napi::Value res) {
            try {
                return JSConvert<bool>::from_value(res);
            }
            catch (const MaaNodeException& exc) {
                std::cerr << exc.what() << std::endl;
                return false;
            }
        });

    return res;
}

inline MaaBool CustomControllerStopApp(const char* intent, void* trans_arg)
{
    auto ctx = reinterpret_cast<CallbackContext*>(trans_arg);

    auto res = ctx->Call<bool>(
        [=](auto env, auto fn) { return fn.Call({ Napi::String::New(env, "stop_app"), Napi::String::New(env, intent) }); },
        [](Napi::Value res) {
            try {
                return JSConvert<bool>::from_value(res);
            }
            catch (const MaaNodeException& exc) {
                std::cerr << exc.what() << std::endl;
                return false;
            }
        });

    return res;
}

inline MaaBool CustomControllerScreencap(void* trans_arg, MaaImageBuffer* buffer)
{
    auto ctx = reinterpret_cast<CallbackContext*>(trans_arg);

    auto res = ctx->Call<std::optional<std::vector<uint8_t>>>(
        [=](auto env, auto fn) { return fn.Call({ Napi::String::New(env, "screencap") }); },
        [](Napi::Value res) -> std::optional<std::vector<uint8_t>> {
            try {
                auto arrayBuffer = JSConvert<std::optional<Napi::ArrayBuffer>>::from_value(res);
                if (arrayBuffer) {
                    auto ptr = static_cast<uint8_t*>(arrayBuffer->Data());
                    return std::vector<uint8_t>(ptr, ptr + arrayBuffer->ByteLength());
                }
                else {
                    return std::nullopt;
                }
            }
            catch (const MaaNodeException& exc) {
                std::cerr << exc.what() << std::endl;
                return std::nullopt;
            }
        });

    if (res) {
        return MaaImageBufferSetEncoded(buffer, res->data(), res->size());
    }
    else {
        return false;
    }
}

inline MaaBool CustomControllerClick(int32_t x, int32_t y, void* trans_arg)
{
    auto ctx = reinterpret_cast<CallbackContext*>(trans_arg);

    auto res = ctx->Call<bool>(
        [=](auto env, auto fn) {
            return fn.Call({ Napi::String::New(env, "click"), Napi::Number::New(env, x), Napi::Number::New(env, y) });
        },
        [](Napi::Value res) {
            try {
                return JSConvert<bool>::from_value(res);
            }
            catch (const MaaNodeException& exc) {
                std::cerr << exc.what() << std::endl;
                return false;
            }
        });

    return res;
}

inline MaaBool CustomControllerSwipe(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration, void* trans_arg)
{
    auto ctx = reinterpret_cast<CallbackContext*>(trans_arg);

    auto res = ctx->Call<bool>(
        [=](auto env, auto fn) {
            return fn.Call(
                { Napi::String::New(env, "swipe"),
                  Napi::Number::New(env, x1),
                  Napi::Number::New(env, y1),
                  Napi::Number::New(env, x2),
                  Napi::Number::New(env, y2),
                  Napi::Number::New(env, duration) });
        },
        [](Napi::Value res) {
            try {
                return JSConvert<bool>::from_value(res);
            }
            catch (const MaaNodeException& exc) {
                std::cerr << exc.what() << std::endl;
                return false;
            }
        });

    return res;
}

inline MaaBool CustomControllerTouchDown(int32_t contact, int32_t x, int32_t y, int32_t pressure, void* trans_arg)
{
    auto ctx = reinterpret_cast<CallbackContext*>(trans_arg);

    auto res = ctx->Call<bool>(
        [=](auto env, auto fn) {
            return fn.Call(
                { Napi::String::New(env, "touch_down"),
                  Napi::Number::New(env, contact),
                  Napi::Number::New(env, y),
                  Napi::Number::New(env, x),
                  Napi::Number::New(env, pressure) });
        },
        [](Napi::Value res) {
            try {
                return JSConvert<bool>::from_value(res);
            }
            catch (const MaaNodeException& exc) {
                std::cerr << exc.what() << std::endl;
                return false;
            }
        });

    return res;
}

inline MaaBool CustomControllerTouchMove(int32_t contact, int32_t x, int32_t y, int32_t pressure, void* trans_arg)
{
    auto ctx = reinterpret_cast<CallbackContext*>(trans_arg);

    auto res = ctx->Call<bool>(
        [=](auto env, auto fn) {
            return fn.Call(
                { Napi::String::New(env, "touch_move"),
                  Napi::Number::New(env, contact),
                  Napi::Number::New(env, y),
                  Napi::Number::New(env, x),
                  Napi::Number::New(env, pressure) });
        },
        [](Napi::Value res) {
            try {
                return JSConvert<bool>::from_value(res);
            }
            catch (const MaaNodeException& exc) {
                std::cerr << exc.what() << std::endl;
                return false;
            }
        });

    return res;
}

inline MaaBool CustomControllerTouchUp(int32_t contact, void* trans_arg)
{
    auto ctx = reinterpret_cast<CallbackContext*>(trans_arg);

    auto res = ctx->Call<bool>(
        [=](auto env, auto fn) { return fn.Call({ Napi::String::New(env, "touch_up"), Napi::Number::New(env, contact) }); },
        [](Napi::Value res) {
            try {
                return JSConvert<bool>::from_value(res);
            }
            catch (const MaaNodeException& exc) {
                std::cerr << exc.what() << std::endl;
                return false;
            }
        });

    return res;
}

inline MaaBool CustomControllerPressKey(int32_t keycode, void* trans_arg)
{
    auto ctx = reinterpret_cast<CallbackContext*>(trans_arg);

    auto res = ctx->Call<bool>(
        [=](auto env, auto fn) { return fn.Call({ Napi::String::New(env, "press_key"), Napi::Number::New(env, keycode) }); },
        [](Napi::Value res) {
            try {
                return JSConvert<bool>::from_value(res);
            }
            catch (const MaaNodeException& exc) {
                std::cerr << exc.what() << std::endl;
                return false;
            }
        });

    return res;
}

inline MaaBool CustomControllerInputText(const char* text, void* trans_arg)
{
    auto ctx = reinterpret_cast<CallbackContext*>(trans_arg);

    auto res = ctx->Call<bool>(
        [=](auto env, auto fn) { return fn.Call({ Napi::String::New(env, "input_text"), Napi::String::New(env, text) }); },
        [](Napi::Value res) {
            try {
                return JSConvert<bool>::from_value(res);
            }
            catch (const MaaNodeException& exc) {
                std::cerr << exc.what() << std::endl;
                return false;
            }
        });

    return res;
}
