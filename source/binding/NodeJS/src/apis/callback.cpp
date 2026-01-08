#include "callback.h"

#include "../foundation/spec.h"

#include "buffer.h"
#include "context.h"
#include "controller.h"
#include "convert.h"
#include "resource.h"
#include "tasker.h"

template <size_t N>
const char* RemovePrefix(const char* message, const char (&prefix)[N])
{
    if (!strncmp(message, prefix, N - 1)) {
        return message + N - 1;
    }
    else {
        return message;
    }
}

void ResourceSink(void* resource, const char* message, const char* details_json, void* callback_arg)
{
    auto ctx = reinterpret_cast<maajs::CallbackContext*>(callback_arg);
    ctx->Call<void>([=](maajs::FunctionType fn) {
        auto res = ResourceImpl::locate_object(fn.Env(), reinterpret_cast<MaaResource*>(resource));
        auto detail = maajs::JsonParse(fn.Env(), details_json).As<maajs::ObjectType>();
        detail["msg"] = maajs::StringType::New(fn.Env(), RemovePrefix(message, "Resource."));
        return fn.Call(
            {
                res,
                detail,
            });
    });
}

void ControllerSink(void* controller, const char* message, const char* details_json, void* callback_arg)
{
    auto ctx = reinterpret_cast<maajs::CallbackContext*>(callback_arg);
    ctx->Call<void>([=](maajs::FunctionType fn) {
        auto ctrl = ControllerImpl::locate_object(fn.Env(), reinterpret_cast<MaaController*>(controller));
        auto detail = maajs::JsonParse(fn.Env(), details_json).As<maajs::ObjectType>();
        detail["msg"] = maajs::StringType::New(fn.Env(), RemovePrefix(message, "Controller."));
        return fn.Call(
            {
                ctrl,
                detail,
            });
    });
}

void TaskerSink(void* tasker, const char* message, const char* details_json, void* callback_arg)
{
    auto ctx = reinterpret_cast<maajs::CallbackContext*>(callback_arg);
    ctx->Call<void>([=](maajs::FunctionType fn) {
        auto tsk = TaskerImpl::locate_object(fn.Env(), reinterpret_cast<MaaTasker*>(tasker));
        auto detail = maajs::JsonParse(fn.Env(), details_json).As<maajs::ObjectType>();
        detail["msg"] = maajs::StringType::New(fn.Env(), RemovePrefix(message, "Tasker."));
        return fn.Call(
            {
                tsk,
                detail,
            });
    });
}

void ContextSink(void* context, const char* message, const char* details_json, void* callback_arg)
{
    auto ctx = reinterpret_cast<maajs::CallbackContext*>(callback_arg);
    ctx->Call<void>([=](maajs::FunctionType fn) {
        auto ctx = ContextImpl::locate_object(fn.Env(), reinterpret_cast<MaaContext*>(context));
        auto detail = maajs::JsonParse(fn.Env(), details_json).As<maajs::ObjectType>();
        detail["msg"] = maajs::StringType::New(fn.Env(), RemovePrefix(message, "Node."));
        return fn.Call(
            {
                ctx,
                detail,
            });
    });
}

MaaBool CustomReco(
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
    using Ret = std::optional<std::tuple<MaaRect, std::string>>;
    auto ctx = reinterpret_cast<maajs::CallbackContext*>(trans_arg);
    auto result = ctx->Call<Ret>([&](maajs::FunctionType func) {
        auto env = func.Env();
        auto self = maajs::ObjectType::New(env);

        self["context"] = ContextImpl::locate_object(env, context);
        self["id"] = maajs::JSConvert<MaaTaskId>::to_value(env, task_id);
        self["task"] = maajs::StringType::New(env, node_name);
        self["name"] = maajs::StringType::New(env, custom_recognition_name);
        self["param"] = maajs::JsonParse(env, custom_recognition_param);
        self["image"] = ImageBufferRefer(image).data(env);
        self["roi"] = maajs::JSConvert<MaaRect>::to_value(env, *roi);

        return func.Call(self, { self });
    });
    if (result) {
        *out_box = std::get<0>(*result);
        StringBuffer(out_detail, false).set(std::get<1>(*result));
        return true;
    }
    else {
        return false;
    }
}

MaaBool CustomAct(
    MaaContext* context,
    MaaTaskId task_id,
    const char* node_name,
    const char* custom_action_name,
    const char* custom_action_param,
    MaaRecoId reco_id,
    const MaaRect* box,
    void* trans_arg)
{
    using Ret = bool;
    auto ctx = reinterpret_cast<maajs::CallbackContext*>(trans_arg);
    return ctx->Call<Ret>([&](maajs::FunctionType func) {
        auto env = func.Env();
        auto self = maajs::ObjectType::New(env);

        self["context"] = ContextImpl::locate_object(env, context);
        self["id"] = maajs::JSConvert<MaaTaskId>::to_value(env, task_id);
        self["task"] = maajs::StringType::New(env, node_name);
        self["name"] = maajs::StringType::New(env, custom_action_name);
        self["param"] = maajs::JsonParse(env, custom_action_param);
        self["recoId"] = maajs::JSConvert<MaaRecoId>::to_value(env, reco_id);
        self["box"] = maajs::JSConvert<MaaRect>::to_value(env, *box);

        return func.Call(self, { self });
    });
}

MaaBool CustomConnect(void* trans_arg)
{
    auto customCtx = reinterpret_cast<CustomControllerContext*>(trans_arg);
    auto ctx = customCtx->callbacks["connect"];
    return ctx->Call<bool>([&](maajs::FunctionType func) { return func.Call({}); });
}

MaaBool CustomConnected(void* trans_arg)
{
    auto customCtx = reinterpret_cast<CustomControllerContext*>(trans_arg);
    auto it = customCtx->callbacks.find("connected");
    if (it == customCtx->callbacks.end() || !it->second) {
        // 回调未提供时默认返回 true（已连接）
        return true;
    }
    return it->second->Call<bool>([&](maajs::FunctionType func) { return func.Call({}); });
}

MaaBool CustomRequestUuid(void* trans_arg, MaaStringBuffer* buffer)
{
    auto customCtx = reinterpret_cast<CustomControllerContext*>(trans_arg);
    auto ctx = customCtx->callbacks["request_uuid"];
    auto result = ctx->Call<std::optional<std::string>>([&](maajs::FunctionType func) { return func.Call({}); });
    if (result) {
        StringBuffer(buffer, false).set(*result);
        return true;
    }
    else {
        return false;
    }
}

MaaControllerFeature CustomGetFeatures(void* trans_arg)
{
    using Ret = std::optional<std::vector<std::string>>;
    auto customCtx = reinterpret_cast<CustomControllerContext*>(trans_arg);
    auto ctx = customCtx->callbacks["get_features"];
    auto result = ctx->Call<Ret>([&](maajs::FunctionType func) { return func.Call({}); });
    if (!result) {
        return 0;
    }

    MaaControllerFeature ret = 0;
    for (auto key : *result) {
        if (key == "mouse") {
            ret |= MaaControllerFeature_UseMouseDownAndUpInsteadOfClick;
        }
        else if (key == "keyboard") {
            ret |= MaaControllerFeature_UseKeyboardDownAndUpInsteadOfClick;
        }
    }
    return ret;
}

MaaBool CustomStartApp(const char* intent, void* trans_arg)
{
    auto customCtx = reinterpret_cast<CustomControllerContext*>(trans_arg);
    auto ctx = customCtx->callbacks["start_app"];
    return ctx->Call<bool>([&](maajs::FunctionType func) {
        return func.Call(
            {
                maajs::StringType::New(func.Env(), intent),
            });
    });
}

MaaBool CustomStopApp(const char* intent, void* trans_arg)
{
    auto customCtx = reinterpret_cast<CustomControllerContext*>(trans_arg);
    auto ctx = customCtx->callbacks["stop_app"];
    return ctx->Call<bool>([&](maajs::FunctionType func) {
        return func.Call(
            {
                maajs::StringType::New(func.Env(), intent),
            });
    });
}

MaaBool CustomScreencap(void* trans_arg, MaaImageBuffer* buffer)
{
    auto customCtx = reinterpret_cast<CustomControllerContext*>(trans_arg);
    auto ctx = customCtx->callbacks["screencap"];
    return ctx->Call<bool>(
        [&](maajs::FunctionType func) { return func.Call({}); },
        [buffer](maajs::ValueType result) {
            try {
                auto data = maajs::JSConvert<std::optional<maajs::ArrayBufferType>>::from_value(result);
                if (data) {
                    ImageBuffer(buffer, false).set(*data);
                    return true;
                }
                else {
                    return false;
                }
            }
            catch (const maajs::MaaError& err) {
                std::cerr << err.what() << std::endl;
                return false;
            }
        });
}

MaaBool CustomClick(int32_t x, int32_t y, void* trans_arg)
{
    auto customCtx = reinterpret_cast<CustomControllerContext*>(trans_arg);
    auto ctx = customCtx->callbacks["click"];
    return ctx->Call<bool>([&](maajs::FunctionType func) {
        return func.Call(
            {
                maajs::NumberType::New(func.Env(), x),
                maajs::NumberType::New(func.Env(), y),
            });
    });
}

MaaBool CustomSwipe(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration, void* trans_arg)
{
    auto customCtx = reinterpret_cast<CustomControllerContext*>(trans_arg);
    auto ctx = customCtx->callbacks["swipe"];
    return ctx->Call<bool>([&](maajs::FunctionType func) {
        return func.Call(
            {
                maajs::NumberType::New(func.Env(), x1),
                maajs::NumberType::New(func.Env(), y1),
                maajs::NumberType::New(func.Env(), x2),
                maajs::NumberType::New(func.Env(), y2),
                maajs::NumberType::New(func.Env(), duration),
            });
    });
}

MaaBool CustomTouchDown(int32_t contact, int32_t x, int32_t y, int32_t pressure, void* trans_arg)
{
    auto customCtx = reinterpret_cast<CustomControllerContext*>(trans_arg);
    auto ctx = customCtx->callbacks["touch_down"];
    return ctx->Call<bool>([&](maajs::FunctionType func) {
        return func.Call(
            {
                maajs::NumberType::New(func.Env(), contact),
                maajs::NumberType::New(func.Env(), x),
                maajs::NumberType::New(func.Env(), y),
                maajs::NumberType::New(func.Env(), pressure),
            });
    });
}

MaaBool CustomTouchMove(int32_t contact, int32_t x, int32_t y, int32_t pressure, void* trans_arg)
{
    auto customCtx = reinterpret_cast<CustomControllerContext*>(trans_arg);
    auto ctx = customCtx->callbacks["touch_move"];
    return ctx->Call<bool>([&](maajs::FunctionType func) {
        return func.Call(
            {
                maajs::NumberType::New(func.Env(), contact),
                maajs::NumberType::New(func.Env(), x),
                maajs::NumberType::New(func.Env(), y),
                maajs::NumberType::New(func.Env(), pressure),
            });
    });
}

MaaBool CustomTouchUp(int32_t contact, void* trans_arg)
{
    auto customCtx = reinterpret_cast<CustomControllerContext*>(trans_arg);
    auto ctx = customCtx->callbacks["touch_up"];
    return ctx->Call<bool>([&](maajs::FunctionType func) {
        return func.Call(
            {
                maajs::NumberType::New(func.Env(), contact),
            });
    });
}

MaaBool CustomClickKey(int32_t keycode, void* trans_arg)
{
    auto customCtx = reinterpret_cast<CustomControllerContext*>(trans_arg);
    auto ctx = customCtx->callbacks["click_key"];
    return ctx->Call<bool>([&](maajs::FunctionType func) {
        return func.Call(
            {
                maajs::NumberType::New(func.Env(), keycode),
            });
    });
}

MaaBool CustomInputText(const char* text, void* trans_arg)
{
    auto customCtx = reinterpret_cast<CustomControllerContext*>(trans_arg);
    auto ctx = customCtx->callbacks["input_text"];
    return ctx->Call<bool>([&](maajs::FunctionType func) {
        return func.Call(
            {
                maajs::StringType::New(func.Env(), text),
            });
    });
}

MaaBool CustomKeyDown(int32_t keycode, void* trans_arg)
{
    auto customCtx = reinterpret_cast<CustomControllerContext*>(trans_arg);
    auto ctx = customCtx->callbacks["key_down"];
    return ctx->Call<bool>([&](maajs::FunctionType func) {
        return func.Call(
            {
                maajs::NumberType::New(func.Env(), keycode),
            });
    });
}

MaaBool CustomKeyUp(int32_t keycode, void* trans_arg)
{
    auto customCtx = reinterpret_cast<CustomControllerContext*>(trans_arg);
    auto ctx = customCtx->callbacks["key_up"];
    return ctx->Call<bool>([&](maajs::FunctionType func) {
        return func.Call(
            {
                maajs::NumberType::New(func.Env(), keycode),
            });
    });
}
