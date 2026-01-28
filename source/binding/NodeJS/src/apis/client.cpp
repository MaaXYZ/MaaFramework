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

ClientImpl::ClientImpl(uint16_t port)
{
    client = MaaAgentClientCreateTcp(port);
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

void ClientImpl::register_resource_sink(maajs::NativeObject<ResourceImpl> resource)
{
    if (!MaaAgentClientRegisterResourceSink(client, resource->resource)) {
        throw maajs::MaaError { "Client register_resource_sink failed" };
    }
}

void ClientImpl::register_controller_sink(maajs::NativeObject<ControllerImpl> controller)
{
    if (!MaaAgentClientRegisterControllerSink(client, controller->controller)) {
        throw maajs::MaaError { "Client register_controller_sink failed" };
    }
}

void ClientImpl::register_tasker_sink(maajs::NativeObject<TaskerImpl> tasker)
{
    if (!MaaAgentClientRegisterTaskerSink(client, tasker->tasker)) {
        throw maajs::MaaError { "Client register_tasker_sink failed" };
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

std::optional<std::vector<std::string>> ClientImpl::get_custom_recognition_list()
{
    StringListBuffer buffer;
    if (!MaaAgentClientGetCustomRecognitionList(client, buffer)) {
        return std::nullopt;
    }

    return buffer.as_vector([](StringBufferRefer buf) { return buf.str(); });
}

std::optional<std::vector<std::string>> ClientImpl::get_custom_action_list()
{
    StringListBuffer buffer;
    if (!MaaAgentClientGetCustomActionList(client, buffer)) {
        return std::nullopt;
    }

    return buffer.as_vector([](StringBufferRefer buf) { return buf.str(); });
}

std::string ClientImpl::to_string()
{
    return std::format(" handle = {:#018x} ", reinterpret_cast<uintptr_t>(client));
}

ClientImpl* ClientImpl::ctor(const maajs::CallbackInfo& info)
{
    try {
        // 如果第一个参数是数字，则视为端口号创建 TCP Client
        if (info.Length() > 0 && info[0].IsNumber()) {
            auto port = maajs::JSConvert<uint16_t>::from_value(info[0]);
            return new ClientImpl { port };
        }
        // 否则视为标识符创建普通 Client
        auto params = maajs::UnWrapArgs<std::tuple<maajs::OptionalParam<std::string>>>(info);
        return new ClientImpl { std::get<0>(params).value_or("") };
    }
    catch (std::exception&) {
        return nullptr;
    }
}

maajs::ValueType ClientImpl::create_tcp(maajs::EnvType env, maajs::OptionalParam<uint16_t> port)
{
    return maajs::CallCtorHelper(ExtContext::get(env)->clientCtor.value(), port.value_or(0));
}

void ClientImpl::init_proto(maajs::ObjectType proto, maajs::FunctionType ctor)
{
    MAA_BIND_FUNC(proto, "destroy", ClientImpl::destroy);
    MAA_BIND_GETTER(proto, "identifier", ClientImpl::get_identifier);
    MAA_BIND_FUNC(proto, "bind_resource", ClientImpl::bind_resource);
    MAA_BIND_FUNC(proto, "register_resource_sink", ClientImpl::register_resource_sink);
    MAA_BIND_FUNC(proto, "register_controller_sink", ClientImpl::register_controller_sink);
    MAA_BIND_FUNC(proto, "register_tasker_sink", ClientImpl::register_tasker_sink);
    MAA_BIND_FUNC(proto, "connect", ClientImpl::connect);
    MAA_BIND_FUNC(proto, "disconnect", ClientImpl::disconnect);
    MAA_BIND_GETTER(proto, "connected", ClientImpl::get_connected);
    MAA_BIND_GETTER(proto, "alive", ClientImpl::get_alive);
    MAA_BIND_SETTER(proto, "timeout", ClientImpl::set_timeout);
    MAA_BIND_GETTER(proto, "custom_recognition_list", ClientImpl::get_custom_recognition_list);
    MAA_BIND_GETTER(proto, "custom_action_list", ClientImpl::get_custom_action_list);
    MAA_BIND_FUNC(ctor, "create_tcp", ClientImpl::create_tcp);
}

maajs::ValueType load_client(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<ClientImpl>::init(env, ctor);
    ExtContext::get(env)->clientCtor = maajs::PersistentFunction(ctor);
    return ctor;
}
