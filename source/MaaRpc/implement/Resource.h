#pragma once

#include "AtomicMap.h"
#include "MaaFramework/MaaDef.h"
#include "Utility.h"
#include "generated/resource.grpc.pb.h"

class ResourceImpl final : public ::maarpc::Resource::Service
{
public:
    ResourceImpl(std::shared_ptr<UtilityImpl> impl) : utility_impl_(std::move(impl)) {}

    ::grpc::Status create(::grpc::ServerContext* context, const ::maarpc::IdRequest* request,
                          ::maarpc::HandleResponse* response) override;
    ::grpc::Status destroy(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                           ::maarpc::EmptyResponse* response) override;
    ::grpc::Status post_path(::grpc::ServerContext* context, const ::maarpc::HandleStringRequest* request,
                             ::maarpc::IIdResponse* response) override;
    ::grpc::Status status(::grpc::ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                          ::maarpc::StatusResponse* response) override;
    ::grpc::Status wait(::grpc::ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                        ::maarpc::StatusResponse* response) override;
    ::grpc::Status loaded(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                          ::maarpc::BoolResponse* response) override;
    ::grpc::Status hash(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                        ::maarpc::StringResponse* response) override;
    ::grpc::Status task_list(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                             ::maarpc::StringResponse* response) override;

    AtomicMap<MaaResourceHandle>& handles() { return handles_; }

private:
    std::shared_ptr<UtilityImpl> utility_impl_ = nullptr;
    AtomicMap<MaaResourceHandle> handles_;
};
