#pragma once

#include "Conf/Conf.h"

#include "MaaToolKit/MaaToolKitAPI.h"
#include "generated/config.grpc.pb.h"

MAA_NS_BEGIN

class ConfigImpl final : public ::maarpc::Config::Service
{
public:
    ::grpc::Status init(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                        ::maarpc::EmptyResponse* response) override;
    ::grpc::Status uninit(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                          ::maarpc::EmptyResponse* response) override;
};

MAA_NS_END
