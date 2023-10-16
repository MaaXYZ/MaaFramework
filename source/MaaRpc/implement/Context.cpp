#include "Buffer.h"
#include "Config.h"
#include "Controller.h"
#include "Device.h"
#include "Instance.h"
#include "Resource.h"
#include "SyncContext.h"
#include "Utility.h"
#include "Utils/Logger.h"

#include <grpcpp/server_builder.h>

#include "Context.h"

Context::Context()
    : utility_impl_(std::make_shared<UtilityImpl>()), image_impl_(std::make_shared<ImageImpl>()),
      resource_impl_(std::make_shared<ResourceImpl>(utility_impl_)),
      controller_impl_(std::make_shared<ControllerImpl>(utility_impl_, image_impl_)),
      syncctx_impl_(std::make_shared<SyncContextImpl>(image_impl_)),
      instance_impl_(
          std::make_shared<InstanceImpl>(utility_impl_, image_impl_, resource_impl_, controller_impl_, syncctx_impl_)),
      device_impl_(std::make_shared<DeviceImpl>()), config_impl_(std::make_shared<ConfigImpl>())
{}

void Context::reg_service(::grpc::ServerBuilder& builder)
{
    builder.RegisterService(utility_impl_.get());
    builder.RegisterService(image_impl_.get());
    builder.RegisterService(resource_impl_.get());
    builder.RegisterService(controller_impl_.get());
    builder.RegisterService(syncctx_impl_.get());
    builder.RegisterService(instance_impl_.get());
    builder.RegisterService(device_impl_.get());
    builder.RegisterService(config_impl_.get());
}
