#pragma once

namespace grpc
{
class ServerBuilder;
}

struct UtilityImpl;
struct ImageImpl;
struct ResourceImpl;
struct ControllerImpl;
struct SyncContextImpl;
struct InstanceImpl;
struct DeviceImpl;

struct Context
{
    Context();
    Context(const Context&) = delete;
    ~Context();
    Context& operator=(const Context&) = delete;

    void regService(::grpc::ServerBuilder& builder);

    UtilityImpl* utilityImpl;
    ImageImpl* imageImpl;
    ResourceImpl* resourceImpl;
    ControllerImpl* controllerImpl;
    SyncContextImpl* syncctxImpl;
    InstanceImpl* instanceImpl;
    DeviceImpl* deviceImpl;
};
