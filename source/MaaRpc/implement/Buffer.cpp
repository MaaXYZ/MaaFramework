#include "Buffer.h"
#include "MaaFramework/MaaAPI.h"
#include "Macro.h"
#include "Utility.h"
#include "Utils/Logger.h"

using namespace ::grpc;

Status ImageImpl::create(ServerContext* context, const ::maarpc::EmptyRequest* request,
                         ::maarpc::HandleResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = request;

    auto id = make_uuid();
    handles_.add(id, MaaCreateImageBuffer());

    response->set_handle(id);

    return Status::OK;
}

Status ImageImpl::destroy(ServerContext* context, const ::maarpc::HandleRequest* request,
                          ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_DEL_HANDLE

    MaaDestroyImageBuffer(handle);

    return Status::OK;
}

Status ImageImpl::is_empty(ServerContext* context, const ::maarpc::HandleRequest* request,
                           ::maarpc::BoolResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    response->set_bool_(MaaIsImageEmpty(handle));

    return Status::OK;
}

Status ImageImpl::clear(ServerContext* context, const ::maarpc::HandleRequest* request,
                        ::maarpc::EmptyResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    MaaClearImage(handle);

    return Status::OK;
}

Status ImageImpl::info(ServerContext* context, const ::maarpc::HandleRequest* request,
                       ::maarpc::ImageInfoResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    response->set_type(MaaGetImageType(handle));
    response->mutable_size()->set_width(MaaGetImageWidth(handle));
    response->mutable_size()->set_height(MaaGetImageHeight(handle));

    return Status::OK;
}

Status ImageImpl::encoded(ServerContext* context, const ::maarpc::HandleRequest* request,
                          ::maarpc::BufferResponse* response)
{
    LogFunc;
    std::ignore = context;

    MAA_GRPC_REQUIRED(handle)

    MAA_GRPC_GET_HANDLE

    auto size = MaaGetImageEncodedSize(handle);
    std::string* buffer = new std::string;
    buffer->resize(size);
    std::memcpy(buffer->data(), MaaGetImageEncoded(handle), size);

    response->set_allocated_buf(buffer);

    return Status::OK;
}

Status ImageImpl::set_encoded(ServerContext* context, const ::maarpc::HandleBufferRequest* request,
                              ::maarpc::BoolResponse* response)
{
    LogFunc;
    std::ignore = context;
    std::ignore = response;

    MAA_GRPC_REQUIRED(handle)
    MAA_GRPC_REQUIRED(buffer)

    MAA_GRPC_GET_HANDLE

    const auto& buf = request->buffer();
    response->set_bool_(
        MaaSetImageEncoded(handle, reinterpret_cast<uint8_t*>(const_cast<char*>(buf.data())), buf.size()));

    return Status::OK;
}
