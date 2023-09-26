#pragma once

#include "MaaFramework/MaaAPI.h"
#include "utility.grpc.pb.h"

#include <atomic>
#include <map>
#include <semaphore>

class UtilityImpl final : public ::maarpc::Utility::Service
{
public:
    struct CallbackState
    {
        std::binary_semaphore write { 1 };
        std::binary_semaphore finish { 0 };
        ::grpc::ServerWriter<::maarpc::Callback>* writer { nullptr };
    };

    CallbackState* get(uint64_t id);

    ::grpc::Status version(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                           ::maarpc::StringResponse* response) override;
    ::grpc::Status set_global_option(::grpc::ServerContext* context, const ::maarpc::SetGlobalOptionRequest* request,
                                     ::maarpc::EmptyResponse* response) override;
    ::grpc::Status acquire_callback_id(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                                       ::maarpc::IdResponse* response) override;
    ::grpc::Status register_callback(::grpc::ServerContext* context, const ::maarpc::IdRequest* request,
                                     ::grpc::ServerWriter<::maarpc::Callback>* writer) override;
    ::grpc::Status unregister_callback(::grpc::ServerContext* context, const ::maarpc::IdRequest* request,
                                       ::maarpc::EmptyResponse* response) override;
    ::grpc::Status acquire_custom_controller_id(::grpc::ServerContext* context, const ::maarpc::EmptyRequest* request,
                                                ::maarpc::IdResponse* response) override;

private:
    std::atomic<uint64_t> callback_id_counter { 0 };
    std::map<uint64_t, CallbackState*> states;
    std::mutex state_mtx;
};

extern void CallbackImpl(MaaStringView msg, MaaStringView detail, MaaCallbackTransparentArg arg);
