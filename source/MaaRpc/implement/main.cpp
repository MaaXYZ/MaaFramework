#include "Resource.h"
#include "Utility.h"
#include <grpcpp/server_builder.h>

int main()
{
    std::string server_address("0.0.0.0:8080");
    UtilityImpl utilityService;
    ResourceImpl resourceService(&utilityService);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&utilityService);
    builder.RegisterService(&resourceService);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}
