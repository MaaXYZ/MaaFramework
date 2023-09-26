#include "Buffer.h"
#include "Controller.h"
#include "Resource.h"
#include "SyncContext.h"
#include "Utility.h"
#include <grpcpp/server_builder.h>

int main()
{
    std::string server_address("0.0.0.0:8080");
    UtilityImpl utilityService;
    ImageImpl imageService;
    ResourceImpl resourceService(&utilityService);
    ControllerImpl controllerService(&utilityService, &imageService);
    SyncContextImpl syncctxService(&imageService);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&utilityService);
    builder.RegisterService(&imageService);
    builder.RegisterService(&resourceService);
    builder.RegisterService(&controllerService);
    builder.RegisterService(&syncctxService);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}
