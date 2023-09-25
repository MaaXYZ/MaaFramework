#pragma once

#include "MaaFrameworkImpl.h"

#include <map>

class MaaFrameworkResourceImpl final : public maarpc::MaaFrameworkResource::Service
{
public:
    MaaFrameworkResourceImpl(MaaFrameworkImpl* impl) : fwImpl(impl) {}

    grpc::Status create(grpc::ServerContext* context, const maarpc::IdRequest* request,
                        maarpc::HandleResponse* response) override;
    grpc::Status destroy(grpc::ServerContext* context, const maarpc::HandleRequest* request,
                         maarpc::EmptyResponse* response) override;
    grpc::Status post_path(grpc::ServerContext* context, const maarpc::HandleStringRequest* request,
                           maarpc::IdResponse* response) override;
    grpc::Status status(grpc::ServerContext* context, const maarpc::HandleIdRequest* request,
                        maarpc::StatusResponse* response) override;
    grpc::Status wait(grpc::ServerContext* context, const maarpc::HandleIdRequest* request,
                      maarpc::StatusResponse* response) override;
    grpc::Status loaded(grpc::ServerContext* context, const maarpc::HandleRequest* request,
                        maarpc::BoolResponse* response) override;
    grpc::Status hash(grpc::ServerContext* context, const maarpc::HandleRequest* request,
                      maarpc::StringResponse* response) override;

private:
    MaaFrameworkImpl* fwImpl;
    std::atomic<uint64_t> res_id_counter { 0 };
    std::map<uint64_t, MaaResourceHandle> handles;
    std::mutex handles_mtx;
};
