#pragma once

#include "Utility.h"
#include "resource.grpc.pb.h"

#include <map>

class ResourceImpl final : public ::maarpc::Resource::Service
{
public:
    ResourceImpl(UtilityImpl* impl) : uImpl(impl) {}

    ::grpc::Status create(::grpc::ServerContext* context, const ::maarpc::IdRequest* request,
                          ::maarpc::HandleResponse* response) override;
    ::grpc::Status destroy(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                           ::maarpc::EmptyResponse* response) override;
    ::grpc::Status post_path(::grpc::ServerContext* context, const ::maarpc::HandleStringRequest* request,
                             ::maarpc::IdResponse* response) override;
    ::grpc::Status status(::grpc::ServerContext* context, const ::maarpc::HandleIdRequest* request,
                          ::maarpc::StatusResponse* response) override;
    ::grpc::Status wait(::grpc::ServerContext* context, const ::maarpc::HandleIdRequest* request,
                        ::maarpc::StatusResponse* response) override;
    ::grpc::Status loaded(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                          ::maarpc::BoolResponse* response) override;
    ::grpc::Status hash(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                        ::maarpc::StringResponse* response) override;

private:
    UtilityImpl* uImpl;
    std::atomic<uint64_t> res_id_counter { 0 };
    std::map<uint64_t, MaaResourceHandle> handles;
    std::mutex handles_mtx;
};
