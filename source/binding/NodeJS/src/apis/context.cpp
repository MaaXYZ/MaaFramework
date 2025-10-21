#include "context.h"
#include "loader.h"

#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"
#include "buffer.h"
#include "ext.h"
#include "tasker.h"

MAA_JS_NATIVE_CLASS_STATIC_IMPL(ContextImpl)

ContextImpl::ContextImpl(MaaContext* ctx)
    : context(ctx)
{
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
    return maajs::CallCtorHelper(ExtContext::get(env)->contextCtor, std::to_string(reinterpret_cast<uintptr_t>(MaaContextClone(context))));
}

std::string ContextImpl::to_string()
{
    return std::format(" handle = {:#018x} ", reinterpret_cast<uintptr_t>(context));
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
    MAA_BIND_FUNC(proto, "override_pipeline", ContextImpl::override_pipeline);
    MAA_BIND_FUNC(proto, "override_next", ContextImpl::override_next);
    MAA_BIND_FUNC(proto, "get_node_data", ContextImpl::get_node_data);
    MAA_BIND_FUNC(proto, "get_node_data_parsed", ContextImpl::get_node_data_parsed);
    MAA_BIND_GETTER(proto, "task_id", ContextImpl::get_task_id);
    MAA_BIND_GETTER(proto, "tasker", ContextImpl::get_tasker);
    MAA_BIND_FUNC(proto, "clone", ContextImpl::clone);
}

maajs::ValueType load_context(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<ContextImpl>::init(env, ctor);
    ExtContext::get(env)->contextCtor = maajs::PersistentFunction(ctor);
    return ctor;
}
