#pragma once

#include "AtomicMap.h"
#include "MaaFramework/MaaDef.h"
#include "generated/utility.grpc.pb.h"

#include <memory>
#include <semaphore>

class UtilityImpl final : public ::maarpc::Utility::Service
{
public:
    struct CallbackState
    {
        std::binary_semaphore write { 1 };
        std::binary_semaphore finish { 0 };
        ::grpc::ServerReaderWriter<::maarpc::Callback, ::maarpc::CallbackRequest>* stream { nullptr };
    };

public:
    ::grpc::Status version(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                           ::maarpc::StringResponse* response) override;
    ::grpc::Status set_global_option(::grpc::ServerContext* context, const ::maarpc::SetGlobalOptionRequest* request,
                                     ::maarpc::EmptyResponse* response) override;
    ::grpc::Status acquire_id(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                              ::maarpc::IdResponse* response) override;
    ::grpc::Status register_callback(
        ::grpc::ServerContext* context,
        ::grpc::ServerReaderWriter<::maarpc::Callback, ::maarpc::CallbackRequest>* stream) override;
    ::grpc::Status unregister_callback(::grpc::ServerContext* context, const ::maarpc::IdRequest* request,
                                       ::maarpc::EmptyResponse* response) override;

    AtomicMap<std::shared_ptr<CallbackState>>& states() { return states_; }

private:
    AtomicMap<std::shared_ptr<CallbackState>> states_;
};

extern std::string make_uuid();
extern void callback_impl(MaaStringView msg, MaaStringView detail, MaaCallbackTransparentArg arg);
