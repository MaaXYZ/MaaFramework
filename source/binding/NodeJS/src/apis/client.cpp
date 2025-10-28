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

std::string ClientImpl::get_identifier()
{
    StringBuffer buf;
    if (!MaaAgentClientIdentifier(client, buf)) {
        throw maajs::MaaError { "Client get identifier failed" };
    }
    return buf.str();
}

void ClientImpl::bind_resource(maajs::NativeObject<ResourceImpl> resource)
{
    if (!MaaAgentClientBindResource(client, resource->resource)) {
        throw maajs::MaaError { "Client bind_resource failed" };
    }
}

maajs::PromiseType ClientImpl::connect()
{
    auto work = new maajs::AsyncWork<bool>(env, [client = client]() { return MaaAgentClientConnect(client); });
    work->Queue();
    return work->Promise();
}

void ClientImpl::disconnect()
{
    if (!MaaAgentClientDisconnect(client)) {
        throw maajs::MaaError { "Client disconnect failed" };
    }
}

bool ClientImpl::get_connected()
{
    return MaaAgentClientConnected(client);
}

bool ClientImpl::get_alive()
{
    return MaaAgentClientAlive(client);
}

void ClientImpl::set_timeout(uint64_t ms)
{
    if (!MaaAgentClientSetTimeout(client, static_cast<int64_t>(ms))) {
        throw maajs::MaaError { "Client set_timeout failed" };
    }
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
    MAA_BIND_GETTER(proto, "identifier", ClientImpl::get_identifier);
    MAA_BIND_FUNC(proto, "bind_resource", ClientImpl::bind_resource);
    MAA_BIND_FUNC(proto, "connect", ClientImpl::connect);
    MAA_BIND_FUNC(proto, "disconnect", ClientImpl::disconnect);
    MAA_BIND_GETTER(proto, "connected", ClientImpl::get_connected);
    MAA_BIND_GETTER(proto, "alive", ClientImpl::get_alive);
    MAA_BIND_SETTER(proto, "timeout", ClientImpl::set_timeout);
}

maajs::ValueType load_client(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<ClientImpl>::init(env, ctor);
    ExtContext::get(env)->clientCtor = maajs::PersistentFunction(ctor);
    return ctor;
}
