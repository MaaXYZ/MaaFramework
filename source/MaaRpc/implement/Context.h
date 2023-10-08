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
struct ConfigImpl;

struct Context
{
    Context();
    Context(const Context&) = delete;
    ~Context();
    Context& operator=(const Context&) = delete;

    void regService(::grpc::ServerBuilder& builder);

    UtilityImpl* utility_impl;
    ImageImpl* image_impl;
    ResourceImpl* resource_impl;
    ControllerImpl* controller_impl;
    SyncContextImpl* syncctx_impl;
    InstanceImpl* instance_impl;
    DeviceImpl* device_impl;
    ConfigImpl* config_impl;
};
