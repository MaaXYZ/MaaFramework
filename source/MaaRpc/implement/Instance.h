#pragma once

#include "AtomicMap.h"
#include "Buffer.h"
#include "Controller.h"
#include "MaaFramework/Instance/MaaInstance.h"
#include "Resource.h"
#include "SyncContext.h"
#include "Utility.h"
#include "instance.grpc.pb.h"

struct InstanceImpl final : public ::maarpc::Instance::Service
{
    struct CustomRecognizerInfo
    {
        std::string name;
        ::grpc::ServerReaderWriter<::maarpc::CustomRecognizerResponse, ::maarpc::CustomRecognizerRequest>* stream;
        ImageImpl* iImpl;
        SyncContextImpl* sImpl;
        std::binary_semaphore finish { 0 };
    };
    struct CustomActionInfo
    {
        std::string name;
        ::grpc::ServerReaderWriter<::maarpc::CustomActionResponse, ::maarpc::CustomActionRequest>* stream;
        SyncContextImpl* sImpl;
        std::binary_semaphore finish { 0 };
    };

    InstanceImpl(UtilityImpl* uimpl, ImageImpl* iimpl, ResourceImpl* rimpl, ControllerImpl* cimpl,
                 SyncContextImpl* simpl)
        : uImpl(uimpl), iImpl(iimpl), rImpl(rimpl), cImpl(cimpl), sImpl(simpl)
    {}

    ::grpc::Status create(::grpc::ServerContext* context, const ::maarpc::IdRequest* request,
                          ::maarpc::HandleResponse* response) override;
    ::grpc::Status destroy(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                           ::maarpc::EmptyResponse* response) override;
    ::grpc::Status register_custom_recognizer(
        ::grpc::ServerContext* context,
        ::grpc::ServerReaderWriter<::maarpc::CustomRecognizerResponse, ::maarpc::CustomRecognizerRequest>* stream)
        override;
    ::grpc::Status unregister_custom_recognizer(::grpc::ServerContext* context,
                                                const ::maarpc::HandleStringRequest* request,
                                                ::maarpc::EmptyResponse* response) override;
    ::grpc::Status clear_custom_recognizer(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                           ::maarpc::EmptyResponse* response) override;
    ::grpc::Status register_custom_action(
        ::grpc::ServerContext* context,
        ::grpc::ServerReaderWriter<::maarpc::CustomActionResponse, ::maarpc::CustomActionRequest>* stream) override;
    ::grpc::Status unregister_custom_action(::grpc::ServerContext* context,
                                            const ::maarpc::HandleStringRequest* request,
                                            ::maarpc::EmptyResponse* response) override;
    ::grpc::Status clear_custom_action(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                       ::maarpc::EmptyResponse* response) override;
    ::grpc::Status bind_resource(::grpc::ServerContext* context, const ::maarpc::HandleHandleRequest* request,
                                 ::maarpc::EmptyResponse* response) override;
    ::grpc::Status bind_controller(::grpc::ServerContext* context, const ::maarpc::HandleHandleRequest* request,
                                   ::maarpc::EmptyResponse* response) override;
    ::grpc::Status inited(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                          ::maarpc::BoolResponse* response) override;
    ::grpc::Status post_task(::grpc::ServerContext* context, const ::maarpc::InstancePostTaskRequest* request,
                             ::maarpc::IIdResponse* response) override;
    ::grpc::Status set_task_param(::grpc::ServerContext* context, const ::maarpc::InstanceSetTaskParamRequest* request,
                                  ::maarpc::EmptyResponse* response) override;
    ::grpc::Status status(::grpc::ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                          ::maarpc::StatusResponse* response) override;
    ::grpc::Status wait(::grpc::ServerContext* context, const ::maarpc::HandleIIdRequest* request,
                        ::maarpc::StatusResponse* response) override;
    ::grpc::Status all_finished(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                                ::maarpc::BoolResponse* response) override;
    ::grpc::Status stop(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                        ::maarpc::EmptyResponse* response) override;
    ::grpc::Status resource(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                            ::maarpc::HandleRequest* response) override;
    ::grpc::Status controller(::grpc::ServerContext* context, const ::maarpc::HandleRequest* request,
                              ::maarpc::HandleRequest* response) override;

    UtilityImpl* uImpl;
    ImageImpl* iImpl;
    ResourceImpl* rImpl;
    ControllerImpl* cImpl;
    SyncContextImpl* sImpl;
    AtomicMap<MaaInstanceHandle> handles;
    AtomicMap<CustomRecognizerInfo*> recos;
    AtomicMap<MaaInstanceHandle, CustomRecognizerInfo*> recoIdx;
    AtomicMap<CustomActionInfo*> actions;
    AtomicMap<MaaInstanceHandle, CustomActionInfo*> actionIdx;
};
