#include "Config.h"
#include "MaaToolkit/MaaToolkitAPI.h"
#include "Utils/Logger.h"

MAA_RPC_NS_BEGIN

using namespace ::grpc;

Status ConfigImpl::init(ServerContext* context, const ::maarpc::EmptyRequest* request,
                        ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    if (MaaToolkitInit()) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaToolkitInit failed");
    }
}

Status ConfigImpl::uninit(ServerContext* context, const ::maarpc::EmptyRequest* request,
                          ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = request;
    std::ignore = response;

    if (MaaToolkitUninit()) {
        return Status::OK;
    }
    else {
        return Status(UNKNOWN, "MaaToolkitUninit failed");
    }
}

MAA_RPC_NS_END
