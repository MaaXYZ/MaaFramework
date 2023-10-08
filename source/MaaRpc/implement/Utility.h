#pragma once

#include "AtomicMap.h"
#include "MaaFramework/Utility/MaaUtility.h"
#include "utility.grpc.pb.h"

#include <semaphore>

struct UtilityImpl final : public ::maarpc::Utility::Service
{
    struct CallbackState
    {
        std::binary_semaphore write { 1 };
        std::binary_semaphore finish { 0 };
        ::grpc::ServerWriter<::maarpc::Callback>* writer { nullptr };
    };

    ::grpc::Status version(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                           ::maarpc::StringResponse* response) override;
    ::grpc::Status set_global_option(::grpc::ServerContext* context, const ::maarpc::SetGlobalOptionRequest* request,
                                     ::maarpc::EmptyResponse* response) override;
    ::grpc::Status acquire_id(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                              ::maarpc::IdResponse* response) override;
    ::grpc::Status register_callback(::grpc::ServerContext* context, const ::maarpc::IdRequest* request,
                                     ::grpc::ServerWriter<::maarpc::Callback>* writer) override;
    ::grpc::Status unregister_callback(::grpc::ServerContext* context, const ::maarpc::IdRequest* request,
                                       ::maarpc::EmptyResponse* response) override;

    AtomicMap<CallbackState*> states;
};

extern std::string make_uuid();
extern void CallbackImpl(MaaStringView msg, MaaStringView detail, MaaCallbackTransparentArg arg);
