#include "Config.h"

using namespace ::grpc;

Status ConfigImpl::init(ServerContext* context, const ::maarpc::EmptyRequest* request,
                        ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    if (MaaToolKitInit()) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaToolKitInit failed");
    }
}

Status ConfigImpl::uninit(ServerContext* context, const ::maarpc::EmptyRequest* request,
                          ::maarpc::EmptyResponse* response)
{
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    if (MaaToolKitUninit()) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaToolKitUninit failed");
    }
}
