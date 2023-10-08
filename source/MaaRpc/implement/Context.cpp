#include "Buffer.h"
#include "Config.h"
#include "Controller.h"
#include "Device.h"
#include "Instance.h"
#include "Resource.h"
#include "SyncContext.h"
#include "Utility.h"

#include <grpcpp/server_builder.h>

#include "Context.h"

Context::Context()
{
    utility_impl = new UtilityImpl();
    image_impl = new ImageImpl();
    resource_impl = new ResourceImpl(utility_impl);
    controller_impl = new ControllerImpl(utility_impl, image_impl);
    syncctx_impl = new SyncContextImpl(image_impl);
    instance_impl = new InstanceImpl(utility_impl, image_impl, resource_impl, controller_impl, syncctx_impl);
    device_impl = new DeviceImpl();
    config_impl = new ConfigImpl();
}

Context::~Context()
{
    delete config_impl;
    delete device_impl;
    delete instance_impl;
    delete syncctx_impl;
    delete controller_impl;
    delete resource_impl;
    delete image_impl;
    delete utility_impl;
}

void Context::regService(::grpc::ServerBuilder& builder)
{
    builder.RegisterService(utility_impl);
    builder.RegisterService(image_impl);
    builder.RegisterService(resource_impl);
    builder.RegisterService(controller_impl);
    builder.RegisterService(syncctx_impl);
    builder.RegisterService(instance_impl);
    builder.RegisterService(device_impl);
    builder.RegisterService(config_impl);
}
