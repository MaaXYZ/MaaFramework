#include "context.h"
#include "loader.h"

#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"
#include "buffer.h"
#include "convert.h"
#include "ext.h"
#include "tasker.h"

ContextImpl::ContextImpl(MaaContext* ctx)
    : context(ctx)
{
}

maajs::PromiseType ContextImpl::run_task(
    maajs::ValueType self,
    maajs::EnvType,
    std::string entry,
    maajs::OptionalParam<maajs::ValueType> pipeline_override)
{
    auto overr = maajs::JsonStringify(env, pipeline_override.value_or(maajs::ObjectType::New(env)));
    auto worker = new maajs::AsyncWork<MaaTaskId>(env, [context = context, entry, overr]() {
        return MaaContextRunTask(context, entry.c_str(), overr.c_str());
    });
    worker->Queue();

    return maajs::PromiseThen(worker->Promise(), self.As<maajs::ObjectType>(), [](const maajs::CallbackInfo& info, maajs::ObjectType self) {
        auto tasker = self["tasker"].AsValue().As<maajs::ObjectType>();
        return maajs::CallMemberHelper<maajs::ValueType>(tasker, "task_detail", maajs::JSConvert<MaaTaskId>::from_value(info[0]));
    });
}

maajs::PromiseType ContextImpl::run_recognition(
    maajs::ValueType self,
    maajs::EnvType,
    std::string entry,
    maajs::ArrayBufferType image,
    maajs::OptionalParam<maajs::ValueType> pipeline_override)
{
    auto buf = std::make_shared<ImageBuffer>();
    buf->set(image);
    auto overr = maajs::JsonStringify(env, pipeline_override.value_or(maajs::ObjectType::New(env)));
    auto worker = new maajs::AsyncWork<MaaRecoId>(env, [context = context, entry, buf, overr]() {
        return MaaContextRunRecognition(context, entry.c_str(), overr.c_str(), *buf);
    });
    worker->Queue();

    return maajs::PromiseThen(worker->Promise(), self.As<maajs::ObjectType>(), [](const maajs::CallbackInfo& info, maajs::ObjectType self) {
        auto tasker = self["tasker"].AsValue().As<maajs::ObjectType>();
        return maajs::CallMemberHelper<maajs::ValueType>(tasker, "recognition_detail", maajs::JSConvert<MaaRecoId>::from_value(info[0]));
    });
}

maajs::PromiseType ContextImpl::run_action(
    maajs::ValueType self,
    maajs::EnvType,
    std::string entry,
    MaaRect box,
    std::string reco_detail,
    maajs::OptionalParam<maajs::ValueType> pipeline_override)
{
    auto overr = maajs::JsonStringify(env, pipeline_override.value_or(maajs::ObjectType::New(env)));
    auto worker = new maajs::AsyncWork<MaaActId>(env, [context = context, entry, box, reco_detail, overr]() {
        return MaaContextRunAction(context, entry.c_str(), overr.c_str(), &box, reco_detail.c_str());
    });
    worker->Queue();

    return maajs::PromiseThen(worker->Promise(), self.As<maajs::ObjectType>(), [](const maajs::CallbackInfo& info, maajs::ObjectType self) {
        auto tasker = self["tasker"].AsValue().As<maajs::ObjectType>();
        return maajs::CallMemberHelper<maajs::ValueType>(tasker, "action_detail", maajs::JSConvert<MaaActId>::from_value(info[0]));
    });
}

void ContextImpl::override_pipeline(maajs::ValueType pipeline)
{
    auto str = maajs::JsonStringify(env, pipeline);

    if (!MaaContextOverridePipeline(context, str.c_str())) {
        throw maajs::MaaError { "Context override_pipeline failed" };
    }
}

void ContextImpl::override_next(std::string node_name, std::vector<std::string> next_list)
{
    StringListBuffer buffer;
    buffer.set_vector(next_list, [](auto str) {
        StringBuffer buf;
        buf.set(str);
        return buf;
    });
    if (!MaaContextOverrideNext(context, node_name.c_str(), buffer)) {
        throw maajs::MaaError { "Context override_next failed" };
    }
}

void ContextImpl::override_image(std::string image_name, maajs::ArrayBufferType image)
{
    ImageBuffer buffer;
    buffer.set(image);
    if (!MaaContextOverrideImage(context, image_name.c_str(), buffer)) {
        throw maajs::MaaError { "Context override_image failed" };
    }
}

std::optional<std::string> ContextImpl::get_node_data(std::string node_name)
{
    StringBuffer buffer;
    if (!MaaContextGetNodeData(context, node_name.c_str(), buffer)) {
        return std::nullopt;
    }
    return buffer.str();
}

std::optional<maajs::ValueType> ContextImpl::get_node_data_parsed(std::string node_name)
{
    auto json = get_node_data(node_name);
    if (!json) {
        return std::nullopt;
    }
    return maajs::JsonParse(env, *json);
}

MaaTaskId ContextImpl::get_task_id()
{
    return MaaContextGetTaskId(context);
}

maajs::ValueType ContextImpl::get_tasker()
{
    return TaskerImpl::locate_object(env, MaaContextGetTasker(context));
}

maajs::ValueType ContextImpl::clone()
{
    return locate_object(env, MaaContextClone(context));
}

void ContextImpl::set_anchor(std::string anchor_name, std::string node_name)
{
    if (!MaaContextSetAnchor(context, anchor_name.c_str(), node_name.c_str())) {
        throw maajs::MaaError { "Context set_anchor failed" };
    }
}

std::optional<std::string> ContextImpl::get_anchor(std::string anchor_name)
{
    StringBuffer buf;
    if (MaaContextGetAnchor(context, anchor_name.c_str(), buf)) {
        return buf.str();
    }
    else {
        return std::nullopt;
    }
}

uint32_t ContextImpl::get_hit_count(std::string node_name)
{
    MaaSize count = 0;
    if (!MaaContextGetHitCount(context, node_name.c_str(), &count)) {
        throw maajs::MaaError { "Context get_hit_count failed" };
    }
    // uint32应该够了
    return static_cast<uint32_t>(count);
}

void ContextImpl::clear_hit_count(std::string node_name)
{
    if (!MaaContextClearHitCount(context, node_name.c_str())) {
        throw maajs::MaaError { "Context clear_hit_count failed" };
    }
}

std::string ContextImpl::to_string()
{
    return std::format(" handle = {:#018x} ", reinterpret_cast<uintptr_t>(context));
}

maajs::ValueType ContextImpl::locate_object(maajs::EnvType env, MaaContext* ctx)
{
    return maajs::CallCtorHelper(ExtContext::get(env)->contextCtor, std::to_string(reinterpret_cast<uintptr_t>(ctx)));
}

ContextImpl* ContextImpl::ctor(const maajs::CallbackInfo& info)
{
    if (info.Length() == 1) {
        try {
            MaaContext* handle = reinterpret_cast<MaaContext*>(std::stoull(info[0].As<maajs::StringType>().Utf8Value()));
            return new ContextImpl { handle };
        }
        catch (std::exception&) {
            return nullptr;
        }
    }
    return nullptr;
}

void ContextImpl::init_proto(maajs::ObjectType proto, maajs::FunctionType)
{
    MAA_BIND_FUNC(proto, "run_task", ContextImpl::run_task);
    MAA_BIND_FUNC(proto, "run_recognition", ContextImpl::run_recognition);
    MAA_BIND_FUNC(proto, "run_action", ContextImpl::run_action);
    MAA_BIND_FUNC(proto, "override_pipeline", ContextImpl::override_pipeline);
    MAA_BIND_FUNC(proto, "override_next", ContextImpl::override_next);
    MAA_BIND_FUNC(proto, "override_image", ContextImpl::override_image);
    MAA_BIND_FUNC(proto, "get_node_data", ContextImpl::get_node_data);
    MAA_BIND_FUNC(proto, "get_node_data_parsed", ContextImpl::get_node_data_parsed);
    MAA_BIND_GETTER(proto, "task_id", ContextImpl::get_task_id);
    MAA_BIND_GETTER(proto, "tasker", ContextImpl::get_tasker);
    MAA_BIND_FUNC(proto, "clone", ContextImpl::clone);
    MAA_BIND_FUNC(proto, "set_anchor", ContextImpl::set_anchor);
    MAA_BIND_FUNC(proto, "get_anchor", ContextImpl::get_anchor);
    MAA_BIND_FUNC(proto, "get_hit_count", ContextImpl::get_hit_count);
    MAA_BIND_FUNC(proto, "clear_hit_count", ContextImpl::clear_hit_count);
}

maajs::ValueType load_context(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<ContextImpl>::init(env, ctor);
    ExtContext::get(env)->contextCtor = maajs::PersistentFunction(ctor);
    return ctor;
}
