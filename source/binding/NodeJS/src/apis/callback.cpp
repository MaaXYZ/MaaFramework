#include "callback.h"

#include "../foundation/spec.h"

#include "buffer.h"
#include "context.h"
#include "controller.h"
#include "convert.h"
#include "resource.h"
#include "tasker.h"

void ResourceSink(void* resource, const char* message, const char* details_json, void* callback_arg)
{
    auto ctx = reinterpret_cast<maajs::CallbackContext*>(callback_arg);
    ctx->Call<void>(
        [=](maajs::FunctionType fn) {
            auto res = ResourceImpl::locate_object(fn.Env(), reinterpret_cast<MaaResource*>(resource));
            auto detail = maajs::JsonParse(fn.Env(), details_json).As<maajs::ObjectType>();
            detail["msg"] = maajs::StringType::New(fn.Env(), message);
            return fn.Call(
                {
                    res,
                    detail,
                });
        },
        [](auto res) { std::ignore = res; });
}

void ControllerSink(void* controller, const char* message, const char* details_json, void* callback_arg)
{
    auto ctx = reinterpret_cast<maajs::CallbackContext*>(callback_arg);
    ctx->Call<void>(
        [=](maajs::FunctionType fn) {
            auto ctrl = ControllerImpl::locate_object(fn.Env(), reinterpret_cast<MaaController*>(controller));
            auto detail = maajs::JsonParse(fn.Env(), details_json).As<maajs::ObjectType>();
            detail["msg"] = maajs::StringType::New(fn.Env(), message);
            return fn.Call(
                {
                    ctrl,
                    detail,
                });
        },
        [](auto res) { std::ignore = res; });
}

void TaskerSink(void* tasker, const char* message, const char* details_json, void* callback_arg)
{
    auto ctx = reinterpret_cast<maajs::CallbackContext*>(callback_arg);
    ctx->Call<void>(
        [=](maajs::FunctionType fn) {
            auto tsk = TaskerImpl::locate_object(fn.Env(), reinterpret_cast<MaaTasker*>(tasker));
            auto detail = maajs::JsonParse(fn.Env(), details_json).As<maajs::ObjectType>();
            detail["msg"] = maajs::StringType::New(fn.Env(), message);
            return fn.Call(
                {
                    tsk,
                    detail,
                });
        },
        [](auto res) { std::ignore = res; });
}

void ContextSink(void* context, const char* message, const char* details_json, void* callback_arg)
{
    auto ctx = reinterpret_cast<maajs::CallbackContext*>(callback_arg);
    ctx->Call<void>(
        [=](maajs::FunctionType fn) {
            auto ctx = ContextImpl::locate_object(fn.Env(), reinterpret_cast<MaaContext*>(context));
            auto detail = maajs::JsonParse(fn.Env(), details_json).As<maajs::ObjectType>();
            detail["msg"] = maajs::StringType::New(fn.Env(), message);
            return fn.Call(
                {
                    ctx,
                    detail,
                });
        },
        [](auto res) { std::ignore = res; });
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
    auto result = ctx->Call<Ret>(
        [&](maajs::FunctionType func) {
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
        },
        [](maajs::ValueType result) { return maajs::JSConvert<Ret>::from_value(result); });
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
    return ctx->Call<Ret>(
        [&](maajs::FunctionType func) {
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
        },
        [](maajs::ValueType result) { return maajs::JSConvert<Ret>::from_value(result); });
}
