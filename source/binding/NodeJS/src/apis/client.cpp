#include "client.h"
#include "loader.h"

#include "../foundation/spec.h"
#include "buffer.h"
#include "ext.h"

ClientImpl::ClientImpl(std::string identifier)
{
    StringBuffer buf;
    buf.set(identifier);
    client = MaaAgentClientCreateV2(buf);
}

ClientImpl::~ClientImpl()
{
    destroy();
}

void ClientImpl::destroy()
{
    MaaAgentClientDestroy(client);
    client = {};
}

std::string ClientImpl::to_string()
{
    return std::format(" handle = {:#018x} ", reinterpret_cast<uintptr_t>(client));
}

ClientImpl* ClientImpl::ctor(const maajs::CallbackInfo& info)
{
    try {
        auto params = maajs::UnWrapArgs<std::tuple<maajs::OptionalParam<std::string>>>(info);
        return new ClientImpl { std::get<0>(params).value_or("") };
    }
    catch (std::exception&) {
        return nullptr;
    }
}

void ClientImpl::init_proto(maajs::ObjectType proto, maajs::FunctionType)
{
    MAA_BIND_FUNC(proto, "destroy", ClientImpl::destroy);
}

maajs::ValueType load_client(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<ClientImpl>::init(env, ctor);
    ExtContext::get(env)->clientCtor = maajs::PersistentFunction(ctor);
    return ctor;
}
