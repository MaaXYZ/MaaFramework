#include "Buffer.h"
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
    utilityImpl = new UtilityImpl();
    imageImpl = new ImageImpl();
    resourceImpl = new ResourceImpl(utilityImpl);
    controllerImpl = new ControllerImpl(utilityImpl, imageImpl);
    syncctxImpl = new SyncContextImpl(imageImpl);
    instanceImpl = new InstanceImpl(utilityImpl, imageImpl, resourceImpl, controllerImpl);
    deviceImpl = new DeviceImpl();
}

Context::~Context()
{
    delete deviceImpl;
    delete instanceImpl;
    delete syncctxImpl;
    delete controllerImpl;
    delete resourceImpl;
    delete imageImpl;
    delete utilityImpl;
}

void Context::regService(::grpc::ServerBuilder& builder)
{
    builder.RegisterService(utilityImpl);
    builder.RegisterService(imageImpl);
    builder.RegisterService(resourceImpl);
    builder.RegisterService(controllerImpl);
    builder.RegisterService(syncctxImpl);
    builder.RegisterService(instanceImpl);
    builder.RegisterService(deviceImpl);
}
