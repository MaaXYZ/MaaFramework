#pragma once

#include <memory>

namespace grpc
{
class ServerBuilder;
}

class UtilityImpl;
class ImageImpl;
class ResourceImpl;
class ControllerImpl;
class SyncContextImpl;
class InstanceImpl;
class DeviceImpl;
class ConfigImpl;

class Context
{
public:
    Context();
    Context(const Context&) = delete;
    ~Context() = default;
    Context& operator=(const Context&) = delete;

    void reg_service(::grpc::ServerBuilder& builder);

private:
    std::shared_ptr<UtilityImpl> utility_impl_ = nullptr;
    std::shared_ptr<ImageImpl> image_impl_ = nullptr;
    std::shared_ptr<ResourceImpl> resource_impl_ = nullptr;
    std::shared_ptr<ControllerImpl> controller_impl_ = nullptr;
    std::shared_ptr<SyncContextImpl> syncctx_impl_ = nullptr;
    std::shared_ptr<InstanceImpl> instance_impl_ = nullptr;
    std::shared_ptr<DeviceImpl> device_impl_ = nullptr;
    std::shared_ptr<ConfigImpl> config_impl_ = nullptr;
};
