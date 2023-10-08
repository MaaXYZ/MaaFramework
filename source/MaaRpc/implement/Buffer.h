#pragma once

#include "AtomicMap.h"
#include "MaaFramework/Utility/MaaBuffer.h"
#include "buffer.grpc.pb.h"

struct ImageImpl final : public ::maarpc::Image::Service
{
    ::grpc::Status create(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                          ::maarpc::HandleResponse* response) override;
    ::grpc::Status destroy(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                           ::maarpc::EmptyResponse* response) override;
    ::grpc::Status is_empty(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                            ::maarpc::BoolResponse* response) override;
    ::grpc::Status clear(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                         ::maarpc::EmptyResponse* response) override;
    ::grpc::Status info(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                        ::maarpc::ImageInfoResponse* response) override;
    ::grpc::Status encoded(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                           ::maarpc::BufferResponse* response) override;
    ::grpc::Status set_encoded(::grpc::ServerContext* context, const ::maarpc::HandleBufferRequest* request,
                               ::maarpc::BoolResponse* response) override;

    AtomicMap<MaaImageBufferHandle> handles;
};
