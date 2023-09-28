#pragma once

#include "MaaToolKit/MaaToolKitAPI.h"
#include "config.grpc.pb.h"

struct ConfigImpl final : public ::maarpc::Config::Service
{
    ::grpc::Status init(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                        ::maarpc::EmptyResponse* response) override;
    ::grpc::Status uninit(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                          ::maarpc::EmptyResponse* response) override;
};
