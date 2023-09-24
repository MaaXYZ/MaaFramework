#pragma once

#include "Framework.grpc.pb.h"

class MaaFrameworkImpl final : public maarpc::MaaFramework::Service
{
public:
    grpc::Status version(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                         ::maarpc::StringResponse* response) override;
    grpc::Status set_global_option(::grpc::ServerContext* context, const ::maarpc::SetGlobalOptionRequest* request,
                                   ::maarpc::EmptyResponse* response) override;
    grpc::Status acquire_callback_id(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                                     ::maarpc::IdResponse* response) override;
    grpc::Status register_callback(::grpc::ServerContext* context, const ::maarpc::IdRequest* request,
                                   ::grpc::ServerWriter<::maarpc::Callback>* writer) override;
    grpc::Status unregister_callback(::grpc::ServerContext* context, const ::maarpc::IdRequest* request,
                                     ::maarpc::EmptyResponse* response) override;
    grpc::Status acquire_custom_controller_id(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                                              ::maarpc::IdResponse* response) override;
};