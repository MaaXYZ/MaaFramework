#include <iostream>

#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "utility.grpc.pb.h"

int main()
{
    auto channel = grpc::CreateChannel("localhost:8080", grpc::InsecureChannelCredentials());
    std::unique_ptr<::maarpc::Utility::Stub> stub(::maarpc::Utility::NewStub(channel));
    grpc::ClientContext ctx;
    ::maarpc::EmptyRequest req;
    ::maarpc::StringResponse res;
    stub->version(&ctx, req, &res);
    std::cout << res.str() << std::endl;

    return 0;
}
