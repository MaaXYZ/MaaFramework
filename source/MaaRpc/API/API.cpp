#include "../implement/Context.h"
#include "MaaRpc/MaaRpc.h"
#include <grpcpp/server_builder.h>

static std::unique_ptr<grpc::Server> server = nullptr;
static std::unique_ptr<Context> context = nullptr;

void MaaRpcStart(MaaStringView address)
{
    if (server) {
        server->Shutdown();
    }

    std::string server_address(address);

    context = std::make_unique<Context>();

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    context->reg_service(builder);

    server = builder.BuildAndStart();
}

void MaaRpcStop()
{
    if (server) {
        using namespace std::chrono_literals;
        server->Shutdown(std::chrono::system_clock::now() + 3s);
    }
    server = nullptr;
    context = nullptr;
}

void MaaRpcWait()
{
    if (server) {
        server->Wait();
    }
}
