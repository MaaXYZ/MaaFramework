#include "../generated/utility.grpc.pb.h"
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <iostream>

int main()
{
    auto channel = grpc::CreateChannel("0.0.0.0:8080", grpc::InsecureChannelCredentials());
    std::unique_ptr<::maarpc::Utility::Stub> stub(::maarpc::Utility::NewStub(channel));
    grpc::ClientContext ctx;
    ::maarpc::EmptyRequest req;
    ::maarpc::StringResponse res;
    stub->version(&ctx, req, &res);
    std::cout << res.str() << std::endl;
}
